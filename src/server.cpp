#include "server.hpp"

#include <iostream>
#include <ostream>
#include <sys/socket.h>
#include <linux/net_tstamp.h>
#include <netinet/in.h>
#include <cstring>
#include <ifaddrs.h>

#include "signal.hpp"
#include "utils.hpp"

bool isSameIp(char ip[4]) {
    ifaddrs * ifAddrStruct = nullptr;
    const ifaddrs * ifa = nullptr;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            const sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            uint32_t ip_raw = sa_in->sin_addr.s_addr;

            if (std::memcmp(&ip_raw, ip, 4) == 0) {
                if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
                return true;
            }
        }
    }
    if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
    return false;
}

int setupSocket(const Settings& settings) {
    const int sock = socket(AF_INET, settings.mode == UDP ? SOCK_DGRAM : SOCK_STREAM, 0);

    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        exit(-1);
    }

    constexpr int flags = SOF_TIMESTAMPING_RX_SOFTWARE;
    if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &flags, sizeof(flags)) < 0) {
        std::cerr << "Error setting SO_TIMESTAMPING" << std::endl;
        exit(-1);
    }

    constexpr int busy_poll_interval = 50;
    if (setsockopt(sock, SOL_SOCKET, SO_BUSY_POLL, &busy_poll_interval, sizeof(busy_poll_interval)) < 0) {
        std::cerr << "Error setting SO_BUSY_POLL" << std::endl;
        exit(-1);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(settings.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        std::cerr << "Error on binding" << std::endl;
        exit(-1);
    }

    if (settings.mode != UDP) {
        if (listen(sock, SOMAXCONN)) {
            std::cerr << "Error listening for UDP connection" << std::endl;
            exit(-1);
        }
    }

    std::cout << "Started listening on port " << settings.port << std::endl;

    return sock;
}

void runServer(const Settings &settings) {
    int sock, clientSock = 0;
    sockaddr_in sender{};

    if (settings.mode == TCP_STREAM) {
        sock = setupSocket(settings);
        clientSock = accept(sock, nullptr, nullptr);
    }

    while (running) {
        if (settings.mode != TCP_STREAM) {
            sock = setupSocket(settings);
            if (settings.mode == TCP) {
                clientSock = accept(sock, nullptr, nullptr);
            }
        }

        const auto [protocol, timestamp, addr_in] = recvMessage(sock);
        if (protocol.hops < 1) {
            uint64_t timeDifference = ((timestamp->tv_sec * 1000000) + timestamp->tv_nsec / 1000) - protocol.timestamp;
            char buffer[17];
            int index = 0;
            std::memcpy(buffer + index, &timeDifference, 8);
            index += 8;
            std::memcpy(buffer + index, protocol.source, 4);
            index += 4;
            std::memcpy(buffer + index, protocol.destination, 4);
            index += 4;
            std::memcpy(buffer + index, &protocol.hops, 1);

            if (settings.mode != UDP) {
                if (const ssize_t sent = send(clientSock, buffer, 17, 0); sent < 0) {
                    std::cerr << "Error writing to socket" << std::endl;
                    exit(-1);
                }
            } else {
                sendto(sock, buffer, 17, 0, reinterpret_cast<const sockaddr*>(&addr_in), sizeof(addr_in));
            }
        } else {
            char buffer[17];
            char hops = protocol.hops;
            hops -= 1;
            int index = 0;
            std::memcpy(buffer + index, &protocol.timestamp, 8);
            index += 8;
            std::memcpy(buffer + index, protocol.source, 4);
            index += 4;
            std::memcpy(buffer + index, protocol.destination, 4);
            index += 4;
            std::memcpy(buffer + index, &hops, 1);

            if (settings.mode == TCP_STREAM) {
                if (const ssize_t sent = send(clientSock, buffer, 17, 0); sent < 0) {
                    std::cerr << "Error writing to socket" << std::endl;
                    exit(-1);
                }
            } else if (settings.mode == UDP) {
                sendto(sock, buffer, 17, 0, reinterpret_cast<const sockaddr*>(&addr_in), sizeof(addr_in));
            } else {
                const int responseSock = socket(AF_INET, SOCK_STREAM, 0);
                if (responseSock < 0) {
                    std::cerr << "Error creating socket" << std::endl;
                    exit(-1);
                }

                if (connect(responseSock, reinterpret_cast<const sockaddr*>(&addr_in), sizeof(addr_in)) < 0) {
                    std::cerr << "Error connecting to socket" << std::endl;
                    exit(-1);
                }

                send(responseSock, buffer, sizeof(buffer), 0);
            }
        }
    }
}