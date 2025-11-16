#pragma once
#include <netinet/in.h>

struct Protocol {
    std::int32_t size;
    std::int64_t timestamp;
    char hops;
};

struct Message {
    Protocol protocol;
    const timespec* timestamp;
    sockaddr_in sender;
};