#include "ota/firmware_receiver.h"

#include "ota/crc32.h"
#include "ota/protocol.h"
#include "ota/simulated_flash.h"
#include "ota/tcp_socket.h"

#include <array>
#include <stdexcept>

namespace ota {

FirmwareReceiver::FirmwareReceiver(SimulatedFlash& flash) : flash_(flash) {}

void FirmwareReceiver::receive(TcpSocket& socket) {
    std::array<std::uint8_t, Protocol::header_size> header_bytes{};
    socket.receive_exact(header_bytes.data(), header_bytes.size());
    const FirmwareHeader header = Protocol::decode_header(header_bytes);

    if (header.size > flash_.capacity()) {
        throw std::runtime_error("Firmware image exceeds simulated flash capacity.");
    }

    flash_.begin_update();
    Crc32 checksum;
    std::array<std::uint8_t, Protocol::chunk_size> buffer{};
    std::uint64_t remaining = header.size;

    while (remaining > 0) {
        const auto chunk_length = static_cast<std::size_t>(
            remaining < buffer.size() ? remaining : buffer.size());
        socket.receive_exact(buffer.data(), chunk_length);
        flash_.write(buffer.data(), chunk_length);
        checksum.update(buffer.data(), chunk_length);
        remaining -= chunk_length;
    }

    if (checksum.value() != header.crc32) {
        throw std::runtime_error("Firmware CRC32 verification failed.");
    }

    flash_.commit();
    const std::uint8_t response = Protocol::success_response;
    socket.send_all(&response, sizeof(response));
}

}  // namespace ota
