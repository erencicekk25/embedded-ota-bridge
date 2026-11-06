#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ota {

struct FirmwareHeader {
    std::uint64_t size;
    std::uint32_t crc32;
};

class Protocol {
public:
    static constexpr std::size_t header_size = 16;
    static constexpr std::size_t chunk_size = 4096;
    static constexpr std::uint8_t success_response = 0x06;

    static std::array<std::uint8_t, header_size> encode_header(const FirmwareHeader& header);
    static FirmwareHeader decode_header(const std::array<std::uint8_t, header_size>& bytes);
};

}  // namespace ota
