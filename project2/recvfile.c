#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MSB_MASK 0xFFFF0000
#define LSB_MASK 0xFFFF

unsigned int crc32b(char *message) {
   int i, j;
   unsigned int byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (message[i] != 0) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

char csum(char *packet, int cnt) {
    unsigned long pack_sum = 0;
    while(cnt > 0) {
        pack_sum += *packet++;
        if (pack_sum & MSB_MASK) { /* standard checksum */
            pack_sum = pack_sum & LSB_MASK;
            pack_sum++; /* increment sum */
        }
        cnt-=1;
    }
    return (pack_sum & LSB_MASK); /* final bit-wise and */
}

int main(int argc, char *argv[])
{
    // Handle Command Line Inputs
    if (argc != 3)
    {
        printf("Incorrect number of arguments!\n");
        exit(1);
    }
    
    // char* pFlag = argv[1];
    char* port = argv[2];
    unsigned short server_port = atoi(port);
    struct sockaddr_in sin, addr;
    int sockfd;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Create a UDP Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* fill in the address of the server socket */
    memset (&sin, 0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(server_port);

    /* bind server socket to the address */
    if (bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0)
    {
        perror("Failed to bind socket to address");
        exit(EXIT_FAILURE);
    }

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    

    long packet_size = 1078;
    short ack_size = 3;
    // long HEADER_LEN = 78;

    char* recv_buf;
    char ackmsg[2];
    char* ack_packet;
    ack_packet = malloc(ack_size);
    char dir[50];
    char fileName[20];
    // char recv_msg[packet_size - HEADER_LEN];
    short msg_size;
    int packet_count = 0;

    FILE *fp;

    ackmsg[0] = 1;

    char lastID;
    lastID = 1;
    // Receive all the packets
    while(1)
    {
        recv_buf = (char *)malloc(packet_size);
        int count = recvfrom(sockfd, recv_buf, packet_size, MSG_WAITALL, (struct sockaddr *)&addr, &addr_len);
        if (count < 0)
            break;
        int tempcount = 0;
        while (count < packet_size)
        {
            tempcount = recvfrom(sockfd, recv_buf+count, sizeof(recv_buf)-count, MSG_WAITALL, (struct sockaddr *)&addr, &addr_len);
            if (tempcount == -1)
                continue;
            count += tempcount;
        }
        unsigned int crc = crc32b(recv_buf);
        if (crc != 0)
        {
            printf("[recv corrupt packet]\n");
            free(recv_buf);
            continue; // If error detected, discard the packet
        }
        
        // Check sequence number in stop & wait fashion
        char recvID;
        recvID = recv_buf[1];
        
        if (recvID != lastID)
        {
            lastID = recvID;
            printf("[recv data] start %d ACCEPTED\n", count);
        }
        else
        {
            free(recv_buf);
            lastID = recvID;
            printf("[recv data] start %d IGNORED\n", count);
            continue;
        }

        packet_count += 1;
        if (packet_count == 1)
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv); // Set the timeout value

        // Copy the packet to the message
        // memcpy(recv_msg, &recv_buf, packet_size);
        strcpy(dir, "/home/shloksobti/Desktop");
        // memcpy(dir, recv_buf+2, 50);
        msg_size = (short) ntohs(*(short *)(recv_buf+2));

        char recv_msg[msg_size];
        memcpy(fileName, recv_buf+54, 20);
        memcpy(recv_msg, recv_buf+74, msg_size);
        // Save message to the file
        char filePath[70];
        strcpy(filePath, dir);
        strcat(filePath, "/");
        strcat(filePath, fileName);
        char* option = "a";
        if (packet_count == 1)
        {
            // file doesn't exist
            option = "w";
        }
        // printf("file path %s, option %s\n", filePath, option);
        fp = fopen(filePath, option);

        if (!fp)
        {
            // printf("Unable to open file.\n");
            perror("Unable to open file");
            return 1;
        }
        if (fwrite(recv_msg, 1, sizeof(recv_msg), fp) != sizeof(recv_msg))
        {
            perror("Write to file error");
            exit(1);
        }
         // close file
        fclose(fp);

        // Send Ack
        ackmsg[1] = lastID;
        char crc_ack = csum(ackmsg, 2);
        memcpy(ack_packet, ackmsg, 2);
        memcpy(ack_packet+2, &crc_ack, 1);
        int sendcount = sendto(sockfd, (const char *)ack_packet, ack_size, MSG_CONFIRM, (const struct sockaddr *) &addr, sizeof(addr));
        if (sendcount <= 0)
        {
            printf("Error sending!\n");
            return 1;
        }

        free(recv_buf);
    }
    printf("[completed]\n");
    return 0;
}
