#pragma once
#include <atomic>
#include <csignal>

inline std::atomic running{true};

inline void handlesignal(int) {
    running.store(false, std::memory_order_release);
    exit(0);
}

inline void registerSignalHandler() {
    std::signal(SIGINT, handlesignal);
}