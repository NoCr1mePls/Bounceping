# installation

# Protocol
The protocol is the following data packed together is bytes (this is done for easy of processing and performance reasons.)
It is defined as the following:
- Timestamp (8 bytes)
- Source (4 bytes)
- Destination (4 bytes)
- Hops (1 byte)

Timestamp is a unix epoch in microseconds. The source is the client IP address (this does not change between the hops). 
Destination is the server, which this message bounces between. Hops is the amount of hops that still have to be 
processed, this is decreased every bounce.

When there is an odd number of hops, the message will end at the server side and will need to be sent back to the client.
This is done with the same message, however the timestamp will be the total time it took (so the difference) and the hops
will be set to 0.

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
-H : specify the amount of hops
-c : amount of messages per batch
-s : message size
-t : amount of tests
-b : amount of batches per test
-i : interval between tests in seconds
-o : output file for detailed results.
-T : The Threshold for times, to filter out excessively big delays.
-m : Specify the mode (TCP, UDP, TCP_STREAM) (default = TCP_STREAM)
```