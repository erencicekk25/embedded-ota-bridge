#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace ota {

class SimulatedFlash {
public:
    SimulatedFlash(std::filesystem::path image_path, std::uint64_t capacity);
    ~SimulatedFlash();

    SimulatedFlash(const SimulatedFlash&) = delete;
    SimulatedFlash& operator=(const SimulatedFlash&) = delete;

    std::uint64_t capacity() const;
    void begin_update();
    void write(const std::uint8_t* data, std::uint64_t length);
    void commit();

private:
    std::filesystem::path image_path_;
    std::filesystem::path staging_path_;
    std::uint64_t capacity_;
    std::uint64_t bytes_written_ = 0;
    std::ofstream staging_file_;
};

}  // namespace ota
