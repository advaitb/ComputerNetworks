## COMP 556: Project 2

#### Team:

Shlok Singh Sobti (sss10)\
Tianyang Pan (tp36)\
Advait Balaji (ab114)

#### Basic Outline:

We have implemented a reliable file transport protocol using the stop and wait protocol. To ensure our program uses as low memory as possible the sender reads a single stream of data equivalent to the max data size that can be held by one packet and sends it across to the receiver. The sender then waits for an ack from the receiver corresponding to the same packet ID before reading the next stream from the file, if an ack corresponding to the packet ID is not received for a given timeout the sender retransmitts the packet which is consistent with the stop and wait protocol.

#### Packet structure:

The packet has three main components, namely: 

* Header (75 Bytes)
  * ACK or Frame Distinguishing Flag (1 Byte)
  * Frame/Packet ID (2 Bytes)
  * Packet Size (2 Bytes)
  * File Directory (50 Bytes)
  * File Name (20 Bytes)
* Data (25,000 Bytes)
* CRC Code / Checkum (4 Bytes or 1 Byte)
  * The main frame uses a CRC code, while the ack frame uses a checksum scheme to check for corrupted packets.


#### Adaptive Timeout:

We implemented a smoothed adaptive timeout that is updated at the end of every iteration. The timeout is initialized with an arbitrary value and after every send and receive cycle the new RTT is calculated. The new timeout is set as the weighted sum of the most recent RTT and sum of previous RTTs is calculated. The assign the timeout value to the socket we use the SOCKOPT() method.

#### Tests:

We test our implementation on cai (and clear) under various network disturbances like Drop, Delay, Duplicate, Mangle, and Reorder with varying values and our protocol produces correct output under any of the above conditions. The actual performance measures depend on the degree of perturbation.

#### Instructions to Run:
1. Compile: make
2. Start receiver: ./recvfile -p [PORT]
3. Start sender:   ./sendfile -r [HOST IP]:[PORT] -f [DIRECTORY/FILE]

### Notes:
* The recvfile will return (with exit code 0) after 100 seconds of not receiving any packets. Although it takes 100 seconds for it to temrinate, the file transferred is closed and can be accessed.
