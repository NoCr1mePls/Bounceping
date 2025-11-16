#pragma once

#include <optional>
#include <string>
#include <thread>

#include "protocol.hpp"

std::string toLowerCase(std::string input);
int safeStoi(const std::string& input);
bool validateIpAddress(const std::string& ipAddress);
std::optional<int> setupThread(const pthread_t& thread);
bool lockMemory();
Message recvMessage(const int& sock);