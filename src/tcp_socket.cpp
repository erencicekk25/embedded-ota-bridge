#include "ota/tcp_socket.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>

#ifndef _WIN32
#include <cerrno>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace ota {
namespace {

std::string socket_error_message(const std::string& operation) {
#ifdef _WIN32
    return operation + " failed with Winsock error " +
           std::to_string(WSAGetLastError());
#else
    return operation + " failed: " + std::system_category().message(errno);
#endif
}

void close_socket(SocketHandle handle) {
#ifdef _WIN32
    closesocket(handle);
#else
    close(handle);
#endif
}

}  // namespace

SocketRuntime::SocketRuntime() {
#ifdef _WIN32
    WSADATA data{};
    const int result = WSAStartup(MAKEWORD(2, 2), &data);
    if (result != 0) {
        throw std::runtime_error("Winsock initialization failed with error " +
                                 std::to_string(result));
    }
#endif
}

SocketRuntime::~SocketRuntime() {
#ifdef _WIN32
    WSACleanup();
#endif
}

TcpSocket::TcpSocket(SocketHandle handle) : handle_(handle) {}

TcpSocket::~TcpSocket() {
    if (handle_ != invalid_socket) {
        close_socket(handle_);
    }
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept : handle_(other.handle_) {
    other.handle_ = invalid_socket;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        if (handle_ != invalid_socket) {
            close_socket(handle_);
        }
        handle_ = other.handle_;
        other.handle_ = invalid_socket;
    }
    return *this;
}

TcpSocket TcpSocket::connect_to(const std::string& host, std::uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo* addresses = nullptr;

    const auto service = std::to_string(port);
    const int lookup_result = getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses);
    if (lookup_result != 0) {
#ifdef _WIN32
        throw std::runtime_error("Address lookup failed with Winsock error " +
                                 std::to_string(lookup_result));
#else
        throw std::runtime_error("Address lookup failed: " + std::string(gai_strerror(lookup_result)));
#endif
    }

    SocketHandle connected = invalid_socket;
    for (auto* address = addresses; address != nullptr; address = address->ai_next) {
        SocketHandle candidate = socket(address->ai_family, address->ai_socktype,
                                        address->ai_protocol);
        if (candidate == invalid_socket) {
            continue;
        }
        if (connect(candidate, address->ai_addr,
                    static_cast<int>(address->ai_addrlen)) == 0) {
            connected = candidate;
            break;
        }
        close_socket(candidate);
    }
    freeaddrinfo(addresses);

    if (connected == invalid_socket) {
        throw std::runtime_error(socket_error_message("TCP connection"));
    }
    return TcpSocket(connected);
}

TcpSocket TcpSocket::listen_on(std::uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    addrinfo* addresses = nullptr;

    const auto service = std::to_string(port);
    const int lookup_result = getaddrinfo(nullptr, service.c_str(), &hints, &addresses);
    if (lookup_result != 0) {
#ifdef _WIN32
        throw std::runtime_error("Address lookup failed with Winsock error " +
                                 std::to_string(lookup_result));
#else
        throw std::runtime_error("Address lookup failed: " + std::string(gai_strerror(lookup_result)));
#endif
    }

    SocketHandle listener = invalid_socket;
    for (auto* address = addresses; address != nullptr; address = address->ai_next) {
        SocketHandle candidate = socket(address->ai_family, address->ai_socktype,
                                        address->ai_protocol);
        if (candidate == invalid_socket) {
            continue;
        }

        int reuse_address = 1;
        setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse_address), sizeof(reuse_address));
        if (bind(candidate, address->ai_addr,
                 static_cast<int>(address->ai_addrlen)) == 0 &&
            listen(candidate, 1) == 0) {
            listener = candidate;
            break;
        }
        close_socket(candidate);
    }
    freeaddrinfo(addresses);

    if (listener == invalid_socket) {
        throw std::runtime_error(socket_error_message("TCP listener"));
    }
    return TcpSocket(listener);
}

TcpSocket TcpSocket::accept_connection() const {
    const SocketHandle client = accept(handle_, nullptr, nullptr);
    if (client == invalid_socket) {
        throw std::runtime_error(socket_error_message("TCP accept"));
    }
    return TcpSocket(client);
}

void TcpSocket::send_all(const std::uint8_t* data, std::size_t length) const {
    std::size_t sent = 0;
    while (sent < length) {
        const auto amount = static_cast<int>(std::min(
            length - sent, static_cast<std::size_t>(std::numeric_limits<int>::max())));
#ifdef _WIN32
        const int result = send(handle_, reinterpret_cast<const char*>(data + sent), amount, 0);
#else
#ifdef MSG_NOSIGNAL
        const int result = static_cast<int>(
            send(handle_, data + sent, static_cast<std::size_t>(amount), MSG_NOSIGNAL));
#else
        const int result = static_cast<int>(
            send(handle_, data + sent, static_cast<std::size_t>(amount), 0));
#endif
#endif
        if (result <= 0) {
            throw std::runtime_error(socket_error_message("TCP send"));
        }
        sent += static_cast<std::size_t>(result);
    }
}

void TcpSocket::receive_exact(std::uint8_t* data, std::size_t length) const {
    std::size_t received = 0;
    while (received < length) {
        const auto amount = static_cast<int>(std::min(
            length - received, static_cast<std::size_t>(std::numeric_limits<int>::max())));
#ifdef _WIN32
        const int result = recv(handle_, reinterpret_cast<char*>(data + received), amount, 0);
#else
        const int result = static_cast<int>(
            recv(handle_, data + received, static_cast<std::size_t>(amount), 0));
#endif
        if (result == 0) {
            throw std::runtime_error("Peer closed the TCP connection before transfer completed.");
        }
        if (result < 0) {
#ifndef _WIN32
            if (errno == EINTR) {
                continue;
            }
#endif
            throw std::runtime_error(socket_error_message("TCP receive"));
        }
        received += static_cast<std::size_t>(result);
    }
}

}  // namespace ota
