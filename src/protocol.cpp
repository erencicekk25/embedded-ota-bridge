#include "ota/protocol.h"

#include <stdexcept>

namespace ota {
namespace {

constexpr std::array<std::uint8_t, 4> magic{{'O', 'T', 'A', '1'}};

void encode_integer(std::uint8_t* destination, std::uint64_t value, std::size_t length) {
    for (std::size_t index = 0; index < length; ++index) {
        const auto shift = static_cast<unsigned int>((length - index - 1) * 8);
        destination[index] = static_cast<std::uint8_t>(value >> shift);
    }
}

std::uint64_t decode_integer(const std::uint8_t* source, std::size_t length) {
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < length; ++index) {
        value = (value << 8U) | source[index];
    }
    return value;
}

}  // namespace

std::array<std::uint8_t, Protocol::header_size> Protocol::encode_header(
    const FirmwareHeader& header) {
    std::array<std::uint8_t, header_size> bytes{};
    for (std::size_t index = 0; index < magic.size(); ++index) {
        bytes[index] = magic[index];
    }
    encode_integer(bytes.data() + 4, header.size, 8);
    encode_integer(bytes.data() + 12, header.crc32, 4);
    return bytes;
}

FirmwareHeader Protocol::decode_header(
    const std::array<std::uint8_t, header_size>& bytes) {
    for (std::size_t index = 0; index < magic.size(); ++index) {
        if (bytes[index] != magic[index]) {
            throw std::runtime_error("Invalid firmware transfer header.");
        }
    }
    return FirmwareHeader{
        decode_integer(bytes.data() + 4, 8),
        static_cast<std::uint32_t>(decode_integer(bytes.data() + 12, 4))};
}

}  // namespace ota
