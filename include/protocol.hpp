#pragma once
#include <netinet/in.h>

#pragma pack(push,1)
struct Protocol {
    std::uint32_t size;
    std::uint64_t timestamp;
    unsigned char hops;
};
#pragma pack(pop)

struct Message {
    Protocol protocol;
    uint64_t timestamp;
    sockaddr_in sender;
};