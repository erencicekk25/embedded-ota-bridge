#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace ota {

class FirmwareSender {
public:
    static void send_file(const std::string& host,
                          std::uint16_t port,
                          const std::filesystem::path& firmware_path);
};

}  // namespace ota
