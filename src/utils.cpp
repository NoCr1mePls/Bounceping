#include "utils.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>

#include "protocol.hpp"
#include "signal.hpp"

std::string toLowerCase(std::string input) {
    std::ranges::transform(input, input.begin(), tolower);
    return input;
}

int safeStoi(const std::string& input) {
    try {
        return std::stoi(input);
    }
    catch (const std::invalid_argument & e) {
        return -1;
    }
    catch (const std::out_of_range & e) {
        return -1;
    }
}

bool validateIpAddress(const std::string& ipAddress) {
    sockaddr_in sockaddr{};
    return inet_pton(AF_INET, ipAddress.c_str(), &sockaddr.sin_addr) != 0;
}

std::optional<int> setupThread(const pthread_t& thread) {
    sched_param sch_params{};
    sch_params.sched_priority = 80;

    if (pthread_setschedparam(thread, SCHED_FIFO, &sch_params) != 0) {
        std::cerr << "Error setting thread priority" << std::endl;
        std::cerr << "Make sure to run with sudo!" << std::endl;
        return -1;
    }

    cpu_set_t cpu_set{};
    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);

    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpu_set) != 0) {
        std::cerr << "Error setting thread affinity" << std::endl;
        std::cerr << "Make sure to run with sudo!" << std::endl;
        return -1;
    }

    return std::nullopt;
}

bool lockMemory() {
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "Error locking memory" << std::endl;
        std::cerr << "Make sure to run with sudo!" << std::endl;
        return false;
    }
    return true;
}

Message recvMessage(const int& sock) {
    char buffer[13];
    char control[1024];

    iovec iov{};
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);

    sockaddr_in sender{};
    msghdr msg{};
    msg.msg_name = &sender;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    if (const ssize_t bytes = recvmsg(sock, &msg, 0); bytes < 0) {
        std::cerr << "Error reading from socket" << std::endl;
        exit(-1);
    }

    const timespec *timestamp = nullptr;

    for (cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET) {
            switch (cmsg->cmsg_type) {
                case SO_TIMESTAMPING:
                    timestamp = reinterpret_cast<timespec *>(CMSG_DATA(cmsg));
                    break;
                default:
                    break;
            }
        }
    }

    if (timestamp == nullptr) {
        std::cerr << "Error getting kernel timestamp." << std::endl;
        exit(-1);
    }

    Protocol protocol{};

    int index = 0;
    std::memcpy(&protocol.size, buffer + index, 4);
    index += 4;
    std::memcpy(&protocol.timestamp, buffer + index, 8);
    index += 8;
    std::memcpy(&protocol.hops, buffer + index, 1);

    return {protocol, timestamp, sender};
}