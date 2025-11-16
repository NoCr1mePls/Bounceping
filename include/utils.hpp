#pragma once

#include <optional>
#include <string>
#include <thread>

std::string toLowerCase(std::string input);
int safeStoi(const std::string& input);
bool validateIpAddress(const std::string& ipAddress);
std::optional<int> setupThread(const pthread_t& thread);
bool lockMemory();