#include "settings.hpp"
#include <iostream>

void printHelp() {
    std::cout <<
            R"(BouncePing - Network Latency Testing Tool
-----------------------------------------

SERVER USAGE:
  bounceping server [options]

OPTIONS:
  -h        Show this help page
  -p <port> Specify the port to listen on
  -m <mode> Set the server mode: TCP | UDP (Default: TCPthth)


CLIENT USAGE:
  bounceping <destination> [options]

OPTIONS:
  -h             Show this help page
  -p <port>      Specify the destination port
  -H <hops>      The number of hops (1-255)
  -c <count>     Messages per batch
  -s <size>      Message size in bytes (default 13)
  -t <tests>     Number of tests to run
  -b <batches>   Batches per test
  -i <seconds>   Interval between tests
  -o <file>      Output file for detailed results
  -T <ms>        Delay threshold; filter out excessively long responses
  -m <mode>      Set the server mode: TCP | UDP | TCP_STREAM (Default: TCP_STREAM)


EXAMPLES:
  Start a server on port 9000 using UDP mode:
    bounceping server -p 9000 -m UDP

  Run a latency test to 192.168.1.10 with 5 batches of 10 messages:
    bounceping 192.168.1.10 -b 5 -c 10
)" << std::endl;
}
