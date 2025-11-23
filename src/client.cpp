#include "client.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <sys/socket.h>
#include <ostream>
#include <linux/net_tstamp.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

#include "signal.hpp"
#include "utils.hpp"

static int setupSocket(const Settings &settings) {
    const int sock = socket(AF_INET, settings.mode == UDP ? SOCK_DGRAM : SOCK_STREAM, 0);

    if (sock < 0) {
        std::cout << "Error creating socket" << std::endl;
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
    addr.sin_addr.s_addr = inet_addr(settings.ip.c_str());

    if (connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Error connecting to socket: " << strerror(errno) << std::endl;
        exit(-1);
    }

    return sock;
}

void runClient(const Settings &settings) {
    uint64_t runTime = 0;
    int sock = setupSocket(settings);

    std::optional<std::ofstream> outputFile;
    if (!settings.output.empty()) {
        outputFile = std::ofstream(settings.output);
    }

    for (int test = 0; test < settings.tests && running; test++) {
        uint64_t testTime = 0;

        if (outputFile.has_value()) {
            *outputFile << "Test " << test << std::endl;
            *outputFile << "|-------------------------------|" << std::endl;
        }

        std::cout << "Starting Test: " << test << std::endl << std::endl;

        for (int batch = 0; batch < settings.batches && running; batch++) {
            uint64_t batchTime = 0;

            bool doneHopping = false;
            for (int messageCount = 0; messageCount < settings.count && running && !doneHopping; messageCount++) {
                std::vector<unsigned char> buffer(settings.size, 255);
                unsigned char* ptr = buffer.data();

                sockaddr_in local{};
                socklen_t localLength = sizeof(local);
                getsockname(sock, reinterpret_cast<sockaddr *>(&local), &localLength);


                in_addr addr{};
                inet_pton(AF_INET, settings.ip.c_str(), &addr);

                std::uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                int index = 0;
                std::memcpy(ptr + index, &settings.size, 4);
                index += 4;
                std::memcpy(ptr + index, &timestamp, 8);
                index += 8;
                std::memcpy(ptr + index, &settings.hops, 1);

                send(sock, ptr, buffer.size(), 0);

                int hops = settings.hops;

                while (hops > 0) {
                    const std::optional<Message> message = recvMessage(sock);
                    if (message->protocol.hops <= 1) {
                        uint64_t timeDifference = 0;
                        timeDifference = message->timestamp - message->protocol.timestamp;


                        if (settings.threshold > 0) {
                            if (timeDifference > settings.threshold) {
                                batch--;
                                std::cout << "Hit threshold! waiting 5 seconds." << std::endl;
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                            } else {
                                batchTime += timeDifference;
                            }
                        } else {
                            batchTime += timeDifference;
                        }

                        if (outputFile.has_value()) {
                            *outputFile << "Message received. Current batchtime: " << batchTime << "us" << std::endl;
                        }
                        break;
                    } else {
                        std::vector<unsigned char> bounceBuffer(settings.size, 255);
                        unsigned char* bouncePtr = bounceBuffer.data();

                        hops = message->protocol.hops;
                        hops--;
                        messageCount--;

                        int bounceIndex = 0;
                        std::memcpy(bouncePtr + bounceIndex, &message->protocol.size, 4);
                        bounceIndex += 4;
                        std::memcpy(bouncePtr + bounceIndex, &message->protocol.timestamp, 8);
                        bounceIndex += 8;
                        std::memcpy(bouncePtr + bounceIndex, &hops, 1);


                        if (const ssize_t sent = send(sock, bouncePtr, message->protocol.size, 0); sent < 0) {
                            std::cerr << "Error writing to socket" << std::endl;
                            exit(-1);
                        }
                    }
                }
            }


            if (outputFile.has_value()) {
                *outputFile << std::endl;
                *outputFile << "Total message time: " << batchTime << "us" << std::endl;
                *outputFile << "Average message time: " << batchTime / settings.count << "us" << std::endl;
            }
            std::cout << "Total message time for batch " << batch <<  ": " << batchTime << "us" << std::endl;
            std::cout << "Average message time for batch " << batch << ": " << batchTime / settings.count << "us" << std::endl;
            testTime += batchTime;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (outputFile.has_value()) {
            *outputFile << std::endl;
            *outputFile << "Total batch time: " << testTime << "us" << std::endl;
            *outputFile << "Average batch time: " << testTime / settings.batches << "us" << std::endl;
        }
        std::cout << std::endl;
        std::cout << "Total batch time for test " << test << ": " << testTime << "us" << std::endl;
        std::cout << "Average batch time for test " << test << ": " << testTime / settings.batches << "us" << std::endl;


        if (outputFile.has_value()) {
            *outputFile << "|-------------------------------|" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(settings.interval));

        runTime += testTime;
    }

    if (outputFile.has_value()) {
        *outputFile << "==============================================" << std::endl;
        *outputFile << "Total batch time: " << runTime << "us" << std::endl;
        *outputFile << "Average batch time: " << runTime / static_cast<long double>(settings.tests) / static_cast<long double>(1000000.0) << "s" << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Total test time: " << runTime / static_cast<long double>(1000000.0) << "s" << std::endl;
    std::cout << "Average test time: " << runTime / static_cast<long double>(settings.tests) / static_cast<long double>(1000000.0) << "s" << std::endl;

    if (outputFile.has_value()) {
        outputFile->close();
    }

    close(sock);
}
