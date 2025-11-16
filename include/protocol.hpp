#pragma once
#include <netinet/in.h>

struct Protocol {
    std::int32_t size;
    std::int64_t timestamp;
    unsigned char hops;
};

struct Message {
    Protocol protocol;
    uint64_t timestamp;
    sockaddr_in sender;
};