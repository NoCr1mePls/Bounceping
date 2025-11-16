#pragma once
#include <netinet/in.h>

struct Protocol {
    std::int64_t timestamp;
    char source[4];
    char destination[4];
    char hops;
};

struct Message {
    Protocol protocol;
    const timespec* timestamp;
    sockaddr_in sender;
};