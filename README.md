# installation
First clone this GitHub repository to your local machine.
You can then run the following to install the tool.
```sh
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .
```

# Protocol
The protocol is the following data packed together is bytes (this is done for easy of processing and performance reasons.)
It is defined as the following:
- Size (4 bytes)
- Timestamp (8 bytes)
- Hops (1 byte)
- Any filler data (??? bytes)

Timestamp is a unix epoch in microseconds. The source is the client IP address (this does not change between the hops). 
Destination is the server, which this message bounces between. Hops is the amount of hops that still have to be 
processed, this is decreased every bounce.

The tool will always have to send the result back to the client when it's done. This delay is added onto the result.

# Usage
Starting the server for the tool is done with the following command:<br>
`bounceping server [-hpm]`

flags:
```yaml
-h : shows a help page
-p : Specify the port
-m : Specify the mode (TCP, UDP, TCP_STREAM) (default = TCP_STREAM)
```

you can then send a bounceping with the following:<br>
`bounceping <destination> [-hpHcstbioT]`

flags:
```yaml
-h : shows a help page
-p : specify the port
-H : specify the amount of hops (1-255)
-c : amount of messages per batch
-s : message size (default 13)
-t : amount of tests
-b : amount of batches per test
-i : interval between tests in seconds
-o : output file for detailed results.
-T : The Threshold for times, to filter out excessively big delays.
-m : Specify the mode (TCP, UDP, TCP_STREAM) (default = TCP_STREAM)
```