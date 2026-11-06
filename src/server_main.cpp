#include "ota/firmware_receiver.h"
#include "ota/simulated_flash.h"
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

std::uint64_t parse_capacity(const char* value) {
    const auto parsed = std::stoull(value);
    if (parsed == 0) {
        throw std::invalid_argument("Flash capacity must be greater than zero.");
    }
    return parsed;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ota_server <port> <flash-image-path> <capacity-bytes>\n";
        return 2;
    }

    try {
        ota::SocketRuntime runtime;
        const auto port = parse_port(argv[1]);
        ota::SimulatedFlash flash(argv[2], parse_capacity(argv[3]));
        ota::FirmwareReceiver receiver(flash);
        auto listener = ota::TcpSocket::listen_on(port);

        std::cout << "Waiting for firmware on port " << port << "...\n";
        auto client = listener.accept_connection();
        receiver.receive(client);
        std::cout << "Firmware verified and written to " << argv[2] << ".\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "OTA server error: " << error.what() << '\n';
        return 1;
    }
}
