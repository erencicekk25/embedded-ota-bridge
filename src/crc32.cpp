#include "ota/crc32.h"

namespace ota {

void Crc32::update(const std::uint8_t* data, std::size_t length) {
    for (std::size_t index = 0; index < length; ++index) {
        state_ ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0U - (state_ & 1U);
            state_ = (state_ >> 1U) ^ (0xEDB88320U & mask);
        }
    }
}

std::uint32_t Crc32::value() const {
    return state_ ^ 0xFFFFFFFFU;
}

}  // namespace ota
