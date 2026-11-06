#pragma once

#include <cstdint>
#include <filesystem>

namespace ota {

class TcpSocket;
class SimulatedFlash;

class FirmwareReceiver {
public:
    explicit FirmwareReceiver(SimulatedFlash& flash);

    void receive(TcpSocket& socket);

private:
    SimulatedFlash& flash_;
};

}  // namespace ota
