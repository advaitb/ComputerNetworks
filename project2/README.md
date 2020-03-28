### COMP 556: Project 2

##### Team:

Shlok Singh Sobti (sss10)
Tianyang Pan (tp36)
Advait Balaji (ab114)

##### Basic Outline:

We have implemented a reliable file transport protocol using the stop and wait protocol. To ensure our program uses as low memory as possible the sender reads a single stream of data equivalent to the max data size that can be held by one packet and sends it across to the receiver. The sender then waits for an ack from the receiver corresponding to the same packet id before reading the next stream from the file, if an ack corresponding to the packet id is not received for a given timeout we the sender retransmitts the packet again which is consistent with the stop and wait protocol.

##### Packet structure:

The packet has three main components, namely: data (MAX_DATA_SIZE = 32000 Bytes), header which contains packet id, directory name and file name (HEADER_SIZE = 74 Bytes); and a 32 bit CRC code to ensure correctness (CRC_SIZE = 4 Bytes). The ack in contrast, uses 1 byte of checksum and 2 Bytes for the packet id. 

##### Adaptive Timeout:

We also use an adaptive timeout that is updated at the end of every iteration. The timeout is initialized at 20 secs and after every send and receive cycle the new RTT is calculated and a weighted sum of the most recent RTT and sum of previous RTTs is calculated. The timeout is then adjusted to ascertain the receive timeout and sockopt for recvtime is set with the said value


##### Tests:

We test our implementation on cai (and clear) under various network disturbances like Drop, Delay, Duplicate, Mangle, and Reorder with varying values and our protocol produces correct output under any of the above conditions. The actual performance measures depend on the degree of perturbation.

##### Instructions to run:
1. Compile: make
2. Start receiver: ./recvfile -p [PORT]
3. Start sender:   ./sendfile -r [HOST IP]:[PORT] -f [DIRECTORY/FILE]

