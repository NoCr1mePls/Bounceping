#include "server.hpp"

#include <iostream>
#include <ostream>
#include <sys/socket.h>
#include <linux/net_tstamp.h>
#include <netinet/in.h>
#include <cstring>

#include "signal.hpp"
#include "utils.hpp"

static int setupSocket(const Settings& settings) {
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

    if (settings.mode != TCP) {
        sock = setupSocket(settings);
        if (settings.mode == TCP_STREAM) {
            clientSock = accept(sock, nullptr, nullptr);
        }
    }

    while (running) {
        if (settings.mode == TCP) {
            sock = setupSocket(settings);
            clientSock = accept(sock, nullptr, nullptr);
        }

        const auto [protocol, timestamp, addr_in] = recvMessage(sock);
        if (protocol.hops <= 1) {
            uint64_t timeDifference = timestamp->tv_sec * 1000000 + timestamp->tv_nsec / 1000 - protocol.timestamp;
            char buffer[protocol.size];
            std::fill_n(buffer, protocol.size, 255);

            char hops = protocol.hops;
            hops--;

            int index = 0;
            std::memcpy(buffer + index, &protocol.size, 4);
            index += 4;
            std::memcpy(buffer + index, &timeDifference, 8);
            index += 4;
            std::memcpy(buffer + index, &hops, 1);

            if (settings.mode != UDP) {
                if (const ssize_t sent = send(clientSock, buffer, protocol.size, 0); sent < 0) {
                    std::cerr << "Error writing to socket" << std::endl;
                    exit(-1);
                }
            } else {
                sendto(sock, buffer, protocol.size, 0, reinterpret_cast<const sockaddr*>(&addr_in), sizeof(addr_in));
            }
        } else {
            char buffer[protocol.size];
            std::fill_n(buffer, protocol.size, 255);

            char hops = protocol.hops;
            hops--;
            int index = 0;
            std::memcpy(buffer + index, &protocol.size, 4);
            index += 4;
            std::memcpy(buffer + index, &protocol.timestamp, 8);
            index += 4;
            std::memcpy(buffer + index, &hops, 1);

            if (settings.mode == TCP_STREAM) {
                if (const ssize_t sent = send(clientSock, buffer, protocol.size, 0); sent < 0) {
                    std::cerr << "Error writing to socket" << std::endl;
                    exit(-1);
                }
            } else if (settings.mode == UDP) {
                sendto(sock, buffer, protocol.size, 0, reinterpret_cast<const sockaddr*>(&addr_in), sizeof(addr_in));
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

                send(responseSock, buffer, protocol.size, 0);
            }
        }
    }
}