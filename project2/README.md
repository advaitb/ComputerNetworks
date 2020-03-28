##### Team:
Shlok Singh Sobti (sss10)
Tianyang Pan (tp36)
Advait Balaji (ab114)

##### Basic Outline:

We have implemented a reliable file transport protocol using the stop and wait protocol. To ensure our program uses as low memory as possible the sender reads a single stream of data equivalent to the max data size that can be held by one packet and sends it across to the receiver. The sender then waits for an ack from the receiver corresponding to the same packet id before reading the next stream from the file, if an ack corresponding to the packet id is not received for a given timeout we the sender retransmitts the packet again which is consistent with the stop and wait protocol.

##### Packet structure:

The packet has three main components, namely: data (MAX_DATA_SIZE = 32000 Bytes), header which contains packet id, directory name and file name (HEADER_SIZE = 74 Bytes);  and a 32 bit CRC code to ensure correctness (CRC_SIZE = 4 Bytes). The ack in contrast uses 1 byte of checksum and 

