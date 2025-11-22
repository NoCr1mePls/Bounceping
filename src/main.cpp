#include <filesystem>
#include <iostream>
#include <optional>
#include <ostream>
#include <thread>
#include <unistd.h>

#include "client.hpp"
#include "server.hpp"
#include "settings.hpp"
#include "utils.hpp"
#include "signal.hpp"

std::optional<int> getCommandLineOptions(Settings &settings, const int argc, char *argv[]) {
    if (argc <= 1) {
        printHelp();
        return 0;
    }
    const bool isServer = toLowerCase(argv[1]) == "server";

    settings.isServer = isServer;

    const std::string flags = isServer ? "hp:m:" : "hp:m:H:c:s:t:b:i:o:T:";

    if (!isServer) {
        if (validateIpAddress(argv[1])) {
            settings.ip = argv[1];
        } else {
            std::cerr << argv[1] << " is not a valid IP address" << std::endl;
            return -1;
        }
    }

    int opt;
    while ((opt = getopt(argc - 1, argv + 1, flags.c_str())) != -1) {
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
                    return -1;
                }
                break;
            }
            case 'H': {
                if (const int hops = safeStoi(optarg); hops > 0) {
                    settings.hops = static_cast<unsigned char>(hops);
                } else {
                    std::cerr << optarg << " is not a valid number of hops" << std::endl;
                    return -1;
                }
                break;
            }
            case 'c': {
                if (const int count = safeStoi(optarg); count > 0) {
                    settings.count = count;
                } else {
                    std::cerr << optarg << " is not a valid count" << std::endl;
                    return -1;
                }
                break;
            }
            case 's': {
                if (const int size = safeStoi(optarg); size >= 13) {
                    settings.size = size;
                } else {
                    std::cerr << optarg << " is not a valid size" << std::endl;
                    return -1;
                }
                break;
            }
            case 't': {
                if (const int tests = safeStoi(optarg); tests > 0) {
                    settings.tests = tests;
                } else {
                    std::cerr << optarg << " is not a valid number of tests" << std::endl;
                    return -1;
                }
                break;
            }
            case 'b': {
                if (const int batch = safeStoi(optarg); batch > 0) {
                    settings.batches = batch;
                } else {
                    std::cerr << optarg << " is not a valid number of batches" << std::endl;
                    return -1;
                }
                break;
            }
            case 'i': {
                if (const int interval = safeStoi(optarg); interval > 0) {
                    settings.interval = interval;
                } else {
                    std::cerr << optarg << " is not a valid interval" << std::endl;
                    return -1;
                }
                break;
            }
            case 'o': {
                if (std::filesystem::exists(optarg)) {
                    settings.output = optarg;
                } else {
                    std::cerr << optarg << " is not a valid output file" << std::endl;
                    return -1;
                }
                break;
            }
            case 'T': {
                if (const int threshold = safeStoi(optarg); threshold > 0) {
                    settings.threshold = threshold;
                } else {
                    std::cerr << optarg << " is not a valid threshold" << std::endl;
                    return -1;
                }
                break;
            }
            case 'm': {
                if (toLowerCase(optarg) == "tcp") {
                    settings.mode = TCP;
                } else if (toLowerCase(optarg) == "udp") {
                    settings.mode = UDP;
                } else {
                    std::cerr << optarg << " is not a valid mode" << std::endl;
                    return -1;
                }
                break;
            }
            default:
                std::cerr << "Unknown option: " << static_cast<char>(optopt) << "\n";
                return -1;
        }
    }
    return std::nullopt;
}

int main(const int argc, char *argv[]) {
    registerSignalHandler();

    Settings settings;

    if (const std::optional<int> cmdResult = getCommandLineOptions(settings, argc, argv); cmdResult.has_value()) {
        return cmdResult.value();
    }

    lockMemory();

    if (settings.isServer) {
        if (const std::optional<int> thrCliResult = setupThread(pthread_self()); thrCliResult.has_value()) {
            return thrCliResult.value();
        }
        runServer(settings);
    } else {
        if (const std::optional<int> thrCliResult = setupThread(pthread_self()); thrCliResult.has_value()) {
            return thrCliResult.value();
        }
        runClient(settings);
    }
    return 0;
}
