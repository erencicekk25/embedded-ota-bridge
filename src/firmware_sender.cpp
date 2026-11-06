#include "ota/firmware_sender.h"

#include "ota/crc32.h"
#include "ota/protocol.h"
#include "ota/tcp_socket.h"

#include <array>
#include <fstream>
#include <stdexcept>

namespace ota {
namespace {

std::uint32_t calculate_crc32(std::ifstream& firmware) {
    Crc32 checksum;
    std::array<std::uint8_t, Protocol::chunk_size> buffer{};

    while (firmware) {
        firmware.read(reinterpret_cast<char*>(buffer.data()),
                      static_cast<std::streamsize>(buffer.size()));
        const auto count = firmware.gcount();
        if (count > 0) {
            checksum.update(buffer.data(), static_cast<std::size_t>(count));
        }
    }

    if (!firmware.eof()) {
        throw std::runtime_error("Failed to read firmware image.");
    }

    firmware.clear();
    firmware.seekg(0, std::ios::beg);
    if (!firmware) {
        throw std::runtime_error("Failed to rewind firmware image.");
    }

    return checksum.value();
}

}  // namespace

void FirmwareSender::send_file(const std::string& host,
                               std::uint16_t port,
                               const std::filesystem::path& firmware_path) {
    std::ifstream firmware(firmware_path, std::ios::binary);
    if (!firmware) {
        throw std::runtime_error("Cannot open firmware image: " + firmware_path.string());
    }

    firmware.seekg(0, std::ios::end);
    const auto file_size = firmware.tellg();
    if (file_size < 0) {
        throw std::runtime_error("Cannot determine firmware image size.");
    }
    firmware.seekg(0, std::ios::beg);
    if (!firmware) {
        throw std::runtime_error("Cannot rewind firmware image.");
    }

    const auto size = static_cast<std::uint64_t>(file_size);
    const auto crc32 = calculate_crc32(firmware);
    const auto header = Protocol::encode_header(FirmwareHeader{size, crc32});
    TcpSocket socket = TcpSocket::connect_to(host, port);
    socket.send_all(header.data(), header.size());

    std::array<std::uint8_t, Protocol::chunk_size> buffer{};
    std::uint64_t remaining = size;
    while (remaining > 0) {
        const auto chunk_length = static_cast<std::size_t>(
            remaining < buffer.size() ? remaining : buffer.size());
        firmware.read(reinterpret_cast<char*>(buffer.data()),
                      static_cast<std::streamsize>(chunk_length));
        if (firmware.gcount() != static_cast<std::streamsize>(chunk_length)) {
            throw std::runtime_error("Firmware image changed or could not be read completely.");
        }
        socket.send_all(buffer.data(), chunk_length);
        remaining -= chunk_length;
    }

    std::uint8_t response = 0;
    socket.receive_exact(&response, sizeof(response));
    if (response != Protocol::success_response) {
        throw std::runtime_error("Server rejected the firmware update.");
    }
}

}  // namespace ota
