#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
using NativeSocket = int;
#endif

namespace ota {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle invalid_socket = INVALID_SOCKET;
#else
using SocketHandle = NativeSocket;
constexpr SocketHandle invalid_socket = -1;
#endif

class SocketRuntime {
public:
    SocketRuntime();
    ~SocketRuntime();

    SocketRuntime(const SocketRuntime&) = delete;
    SocketRuntime& operator=(const SocketRuntime&) = delete;
};

class TcpSocket {
public:
    explicit TcpSocket(SocketHandle handle = invalid_socket);
    ~TcpSocket();

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    static TcpSocket connect_to(const std::string& host, std::uint16_t port);
    static TcpSocket listen_on(std::uint16_t port);
    TcpSocket accept_connection() const;

    void send_all(const std::uint8_t* data, std::size_t length) const;
    void receive_exact(std::uint8_t* data, std::size_t length) const;

private:
    SocketHandle handle_;
};

}  // namespace ota
