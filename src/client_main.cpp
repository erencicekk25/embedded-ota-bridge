#include "ota/firmware_sender.h"
#include "ota/tcp_socket.h"

#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

std::uint16_t parse_port(const char* value) {
    const auto parsed = std::stoul(value);
    if (parsed == 0 || parsed > std::numeric_limits<std::uint16_t>::max()) {
        throw std::invalid_argument("Port must be between 1 and 65535.");
    }
    return static_cast<std::uint16_t>(parsed);
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ota_client <server-host> <port> <firmware-file>\n";
        return 2;
    }

    try {
        ota::SocketRuntime runtime;
        ota::FirmwareSender::send_file(argv[1], parse_port(argv[2]), argv[3]);
        std::cout << "Firmware transfer completed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "OTA client error: " << error.what() << '\n';
        return 1;
    }
}
