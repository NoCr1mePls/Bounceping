#pragma once

#include <string>

enum Mode {
    TCP,
    UDP,
    TCP_STREAM
};

struct Settings {
    int port = 13234;
    unsigned char hops = 1;
    int count = 1;
    int size = 13;
    Mode mode = TCP_STREAM;
    int tests = 1;
    int batches = 10;
    int interval = 3;
    std::string output;
    int threshold = -1;
    std::string ip;
    bool isServer = false;
};

void printHelp();