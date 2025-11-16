#include <filesystem>
#include <iostream>
#include <optional>
#include <ostream>
#include <thread>
#include <unistd.h>

#include "server.hpp"
#include "settings.hpp"
#include "utils.hpp"

std::optional<int> getCommandLineOptions(Settings& settings, int argc, char* argv[]) {
    if (argc <= 1) {
        printHelp();
        return 0;
    }
    const bool isServer = toLowerCase(argv[1]) == "server";

    const std::string flags = isServer ? "hp:m:" : "hp:H:c:s:t:b:i:o:T:";

    if (!isServer) {
        if (validateIpAddress(argv[1])) {
            settings.ip = argv[1];
        }
    }

    while (const int opt = getopt(argc - 1, argv + 1, flags.c_str()) != -1) {
        switch (opt) {
            case '?':
            case 'h': {
                printHelp();
                return 0;
            }
            case 'p': {
                if (const int port = safeStoi(optarg); port > -1) {
                    settings.port = port;
                } else {
                    std::cerr << optarg << " is not a valid port" << std::endl;
                }
                break;
            }
            case 'H': {
                if (const int hops = safeStoi(optarg); hops > 0) {
                    settings.hops = hops;
                } else {
                    std::cerr << optarg << " is not a valid number of hops" << std::endl;
                }
                break;
            }
            case 'c' : {
                if (const int count = safeStoi(optarg); count > 0) {
                    settings.count = count;
                } else {
                    std::cerr << optarg << " is not a valid count" << std::endl;
                }
                break;
            }
            case 's' : {
                if (const int size = safeStoi(optarg); size > 100) {
                    settings.size = size;
                } else {
                    std::cerr << optarg << " is not a valid size" << std::endl;
                }
                break;
            }
            case 't' : {
                if (const int tests = safeStoi(optarg); tests > 0) {
                    settings.tests = tests;
                } else {
                    std::cerr << optarg << " is not a valid number of tests" << std::endl;
                }
                break;
            }
            case 'b' : {
                if (const int batch = safeStoi(optarg); batch > 0) {
                    settings.batches = batch;
                } else {
                    std::cerr << optarg << " is not a valid number of batches" << std::endl;
                }
                break;
            }
            case 'i' : {
                if (const int interval = safeStoi(optarg); interval > 0) {
                    settings.interval = interval;
                } else {
                    std::cerr << optarg << " is not a valid interval" << std::endl;
                }
                break;
            }
            case 'o' : {
                if (std::filesystem::exists(optarg)) {
                    settings.output = optarg;
                } else {
                    std::cerr << optarg << " is not a valid output file" << std::endl;
                }
                break;
            }
            case 'T' : {
                if (const int threshold = safeStoi(optarg); threshold > 0) {
                    settings.threshold = threshold;
                } else {
                    std::cerr << optarg << " is not a valid threshold" << std::endl;
                }
                break;
            }
            case 'm' : {
                if (toLowerCase(optarg) == "tcp") {
                    settings.mode = TCP;
                } else if (toLowerCase(optarg) == "udp") {
                    settings.mode = UDP;
                } else if (toLowerCase(optarg) == "tcp_stream") {
                    settings.mode = TCP_STREAM;
                } else {
                    std::cerr << optarg << " is not a valid mode" << std::endl;
                }
                break;
            }
            default:
                std::cerr << "Unknown option: " << static_cast<char>(optopt) << "\n";
                break;
        }
    }
    return std::nullopt;
}

int main(const int argc, char *argv[]) {
    Settings settings;

    if (const std::optional<int> cmdResult = getCommandLineOptions(settings, argc, argv); cmdResult.has_value()) {
        return cmdResult.value();
    }

    lockMemory();

    std::thread server(runServer, settings);

    if (const std::optional<int> thrSrvResult = setupThread(server.native_handle()); thrSrvResult.has_value()) {
        return thrSrvResult.value();
    }

    if (const std::optional<int> thrCliResult = setupThread(pthread_self()); thrCliResult.has_value()) {
        return thrCliResult.value();
    }
}