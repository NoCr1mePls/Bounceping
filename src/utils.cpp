#include "utils.hpp"

#include <algorithm>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>

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
        return -1;
    }

    cpu_set_t cpu_set{};
    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);

    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpu_set) != 0) {
        std::cerr << "Error setting thread affinity" << std::endl;
        return 01;
    }

    return std::nullopt;
}

bool lockMemory() {
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "Error locking memory" << std::endl;
        return false;
    }
    return true;
}