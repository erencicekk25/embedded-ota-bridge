#include "ota/simulated_flash.h"

#include <stdexcept>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ota {

SimulatedFlash::SimulatedFlash(std::filesystem::path image_path, std::uint64_t capacity)
    : image_path_(std::move(image_path)),
      staging_path_(image_path_),
      capacity_(capacity) {
    if (image_path_.empty()) {
        throw std::invalid_argument("Flash image path must not be empty.");
    }
    if (capacity_ == 0) {
        throw std::invalid_argument("Flash capacity must be greater than zero.");
    }
    staging_path_ += ".staging";
}

SimulatedFlash::~SimulatedFlash() {
    if (staging_file_.is_open()) {
        staging_file_.close();
    }
    std::error_code ignored;
    std::filesystem::remove(staging_path_, ignored);
}

std::uint64_t SimulatedFlash::capacity() const {
    return capacity_;
}

void SimulatedFlash::begin_update() {
    if (staging_file_.is_open()) {
        staging_file_.close();
    }
    staging_file_.open(staging_path_, std::ios::binary | std::ios::trunc);
    if (!staging_file_) {
        throw std::runtime_error("Cannot open simulated flash staging file: " +
                                 staging_path_.string());
    }
    bytes_written_ = 0;
}

void SimulatedFlash::write(const std::uint8_t* data, std::uint64_t length) {
    if (!staging_file_.is_open()) {
        throw std::logic_error("Flash update has not been started.");
    }
    if (length > capacity_ - bytes_written_) {
        throw std::runtime_error("Firmware image exceeds simulated flash capacity.");
    }
    if (length > static_cast<std::uint64_t>(
                     std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error("Flash write is too large for the file stream.");
    }
    staging_file_.write(reinterpret_cast<const char*>(data),
                        static_cast<std::streamsize>(length));
    if (!staging_file_) {
        throw std::runtime_error("Failed to write simulated flash staging file.");
    }
    bytes_written_ += length;
}

void SimulatedFlash::commit() {
    if (!staging_file_.is_open()) {
        throw std::logic_error("Flash update has not been started.");
    }
    staging_file_.flush();
    if (!staging_file_) {
        throw std::runtime_error("Failed to flush simulated flash staging file.");
    }
    staging_file_.close();
    if (staging_file_.fail()) {
        throw std::runtime_error("Failed to close simulated flash staging file.");
    }

#ifdef _WIN32
    if (!MoveFileExW(staging_path_.c_str(), image_path_.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                "Failed to commit simulated flash image");
    }
#else
    std::error_code error;
    std::filesystem::rename(staging_path_, image_path_, error);
    if (error) {
        throw std::system_error(error, "Failed to commit simulated flash image");
    }
#endif
}

}  // namespace ota
