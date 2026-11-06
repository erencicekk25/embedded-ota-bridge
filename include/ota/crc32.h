#pragma once

#include <cstddef>
#include <cstdint>

namespace ota {

class Crc32 {
public:
    void update(const std::uint8_t* data, std::size_t length);
    std::uint32_t value() const;

private:
    std::uint32_t state_ = 0xFFFFFFFFU;
};

}  // namespace ota
