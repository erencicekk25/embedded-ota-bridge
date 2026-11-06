#include "ota/crc32.h"
#include "ota/protocol.h"
#include "ota/simulated_flash.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

void test_crc32_known_vector() {
    constexpr char input[] = "123456789";
    ota::Crc32 checksum;
    checksum.update(reinterpret_cast<const std::uint8_t*>(input), sizeof(input) - 1);
    if (checksum.value() != 0xCBF43926U) {
        throw std::runtime_error("CRC32 known-vector test failed.");
    }
}

void test_crc32_incremental_updates() {
    constexpr char first[] = "1234";
    constexpr char second[] = "56789";
    ota::Crc32 checksum;
    checksum.update(reinterpret_cast<const std::uint8_t*>(first), sizeof(first) - 1);
    checksum.update(reinterpret_cast<const std::uint8_t*>(second), sizeof(second) - 1);
    if (checksum.value() != 0xCBF43926U) {
        throw std::runtime_error("Incremental CRC32 test failed.");
    }
}

void test_protocol_header_round_trip() {
    const ota::FirmwareHeader expected{0x0102030405060708ULL, 0xA1B2C3D4U};
    const auto encoded = ota::Protocol::encode_header(expected);
    const auto decoded = ota::Protocol::decode_header(encoded);
    if (decoded.size != expected.size || decoded.crc32 != expected.crc32) {
        throw std::runtime_error("Protocol header round-trip test failed.");
    }
}

void test_protocol_rejects_invalid_magic() {
    std::array<std::uint8_t, ota::Protocol::header_size> bytes{};
    bool rejected = false;
    try {
        static_cast<void>(ota::Protocol::decode_header(bytes));
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    if (!rejected) {
        throw std::runtime_error("Protocol accepted an invalid magic value.");
    }
}

void test_simulated_flash_commits_verified_image() {
    const auto unique_id = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto image_path = std::filesystem::temp_directory_path() /
                            ("ota-test-" + std::to_string(unique_id) + ".bin");
    const std::string original = "previous firmware";
    const std::string replacement = "updated firmware";

    try {
        {
            std::ofstream image(image_path, std::ios::binary);
            image << original;
            if (!image) {
                throw std::runtime_error("Could not create simulated flash test image.");
            }
        }

        {
            ota::SimulatedFlash flash(image_path, 64);
            flash.begin_update();
            flash.write(reinterpret_cast<const std::uint8_t*>(replacement.data()),
                        replacement.size());
            flash.commit();
        }

        std::ifstream image(image_path, std::ios::binary);
        const std::string contents((std::istreambuf_iterator<char>(image)),
                                   std::istreambuf_iterator<char>());
        if (contents != replacement) {
            throw std::runtime_error("Simulated flash did not commit the new image.");
        }
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(image_path, ignored);
        throw;
    }

    std::filesystem::remove(image_path);
}

void test_simulated_flash_preserves_image_without_commit() {
    const auto unique_id = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto image_path = std::filesystem::temp_directory_path() /
                            ("ota-test-" + std::to_string(unique_id) + ".bin");
    const std::string original = "previous firmware";
    const std::string incomplete = "partial update";

    try {
        {
            std::ofstream image(image_path, std::ios::binary);
            image << original;
            if (!image) {
                throw std::runtime_error("Could not create simulated flash test image.");
            }
        }

        {
            ota::SimulatedFlash flash(image_path, 64);
            flash.begin_update();
            flash.write(reinterpret_cast<const std::uint8_t*>(incomplete.data()),
                        incomplete.size());
        }

        std::ifstream image(image_path, std::ios::binary);
        const std::string contents((std::istreambuf_iterator<char>(image)),
                                   std::istreambuf_iterator<char>());
        if (contents != original) {
            throw std::runtime_error("An incomplete update replaced the flash image.");
        }
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(image_path, ignored);
        throw;
    }

    std::filesystem::remove(image_path);
}

}  // namespace

int main() {
    try {
        test_crc32_known_vector();
        test_crc32_incremental_updates();
        test_protocol_header_round_trip();
        test_protocol_rejects_invalid_magic();
        test_simulated_flash_commits_verified_image();
        test_simulated_flash_preserves_image_without_commit();
        std::cout << "All OTA tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
