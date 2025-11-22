#include "server.hpp"

#include <iostream>
#include <ostream>
#include <sys/socket.h>
#include <linux/net_tstamp.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>

#include "signal.hpp"
#include "utils.hpp"

static int setupSocket(const Settings &settings) {
    const int sock = socket(AF_INET, settings.mode == UDP ? SOCK_DGRAM : SOCK_STREAM, 0);

    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        exit(-1);
    }

    constexpr int busy_poll_interval = 50;
    if (setsockopt(sock, SOL_SOCKET, SO_BUSY_POLL, &busy_poll_interval, sizeof(busy_poll_interval)) < 0) {
        std::cerr << "Error setting SO_BUSY_POLL" << std::endl;
        exit(-1);
    }

    constexpr int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting SO_REUSEADDR" << std::endl;
        exit(-1);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(settings.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        std::cerr << "Error on binding: " << strerror(errno) << std::endl;
        exit(-1);
    }

    if (settings.mode != UDP) {
        if (listen(sock, SOMAXCONN)) {
            std::cerr << "Error listening for UDP connection: " << strerror(errno) << std::endl;
            exit(-1);
        }
    }

    std::cout << "Started listening on port " << settings.port << std::endl;

    return sock;
}

void runServer(const Settings &settings) {
    int sock = 0;

    while (running) {
        const int clientSock = setupSocket(settings);
        if (settings.mode == TCP) {
            sock = accept(clientSock, nullptr, nullptr);
            close(clientSock);
            constexpr int busy_poll_interval = 50;
            if (setsockopt(sock, SOL_SOCKET, SO_BUSY_POLL, &busy_poll_interval, sizeof(busy_poll_interval)) < 0) {
                std::cerr << "Error setting SO_BUSY_POLL" << std::endl;
                exit(-1);
            }
        } else {
            sock = clientSock;
        }

        while (running) {
            const std::optional<Message> message = recvMessage(sock);

            if (!message.has_value()) {
                break;
            }

            std::vector<unsigned char> buffer(message->protocol.size, 255);
            unsigned char* ptr = buffer.data();


            unsigned char hops = message->protocol.hops;
            hops--;
            int index = 0;
            std::memcpy(ptr + index, &message->protocol.size, sizeof(message->protocol.size));
            index += sizeof(message->protocol.size);

            std::memcpy(ptr + index, &message->protocol.timestamp, sizeof(message->protocol.timestamp));
            index += sizeof(message->protocol.timestamp);

            std::memcpy(ptr + index, &hops, 1);

            if (settings.mode == TCP) {
                if (const ssize_t sent = send(sock, ptr, message->protocol.size, 0); sent < 0) {
                    std::cerr << "Error writing to socket" << std::endl;
                    exit(-1);
                }
            } else {
                sendto(sock, ptr, message->protocol.size, 0, reinterpret_cast<const sockaddr *>(&message->sender), sizeof(message->sender));
            }

            clearSocket(sock);
        }
    }
}