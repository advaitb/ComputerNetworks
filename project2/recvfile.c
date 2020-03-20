#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

unsigned int crc32b(unsigned char *message) {
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

int main(int argc, char const *argv[])
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

    long packet_size = 1076;
    long HEADER_LEN = 76;

    char* recv_buf;
    char ackmsg[2];
    char dir[50];
    char fileName[20];
    char recv_msg[packet_size - HEADER_LEN];
    int packet_count = 0;

    // // Keep waiting for incoming connection
    // while(1)
    // {
    // Accept a new socket
    // new_sock = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
    // if (new_sock < 0)
    // {
    //     perror("error accepting connection");
    //     abort();
    // }

    // // Connection is made
    // printf("Accepted connection. Client IP address: %s\n", inet_ntoa(addr.sin_addr));

    FILE *fp;

    ackmsg[0] = 1;

    char lastID[1];
    lastID[0] = 1;
    // Receive all the packets
    while(1)
    {
        // printf("Packet recv buf Size %d\n", sizeof(recv_buf));
        recv_buf = (char *)malloc(packet_size);
        int count = recvfrom(sockfd, recv_buf, packet_size, MSG_WAITALL, (struct sockaddr *)&addr, &addr_len);
        printf("Received bytes %d\n", count);
        int tempcount = 0;
        while (count < packet_size)
        {
            tempcount = recvfrom(sockfd, recv_buf+count, sizeof(recv_buf)-count, MSG_WAITALL, (struct sockaddr *)&addr, &addr_len);
            printf("Received bytes temp count%d\n", tempcount);
            if (tempcount == -1)
                continue;
            count += tempcount;
        }
        // Check crc
        printf("Check crc\n");
        unsigned int crc = crc32b(recv_buf);
        // printf("crc %d\n", crc);
        if (crc != 0)
        {
            printf("CRC error\n");
            free(recv_buf);
            continue; // If error detected, discard the packet
        }

        // Check sequence number in stop & wait fashion
        printf("Check sequence number\n");
        char recvID[1];
        recvID[0] = recv_buf[1];
        printf("recved packet %d %d\n", recv_buf[0], recv_buf[1]);
        printf("recvID %d\n", recvID[0]);
        printf("lastID %d\n", lastID[0]);
        // printf("last ID:%d, recv ID: %d\n", lastID[0], recvID);
        if (recvID != lastID[0])
            lastID[0] = recvID[0];
        else
        {
            free(recv_buf);
            continue;
        }

        packet_count += 1;

        // Copy the packet to the message
        // memcpy(recv_msg, &recv_buf, packet_size);
        strcpy(dir, "/home/shloksobti/Desktop");
        // strncpy(dir, recv_buf+2, 50);
        memcpy(fileName, recv_buf+52, 20);
        memcpy(recv_msg, recv_buf+72, 1000);

        printf("\nrecv_msg\n");
        for (int i = 0; i < sizeof recv_msg; i ++) {
                printf(" %02x", (unsigned) recv_msg[i]);
        }

        printf("\n\n\nrecv_buf\n");

        for (int i = 0; i < 1076; i ++) {
                printf(" %02x", (unsigned) recv_buf[i]);
        }

        // Save message to the file
        char filePath[70];
        strcpy(filePath, dir);
        strcat(filePath, "/");
        strcat(filePath, fileName);
        char* option = "a";
        // if (access(filePath, F_OK) == -1)
        if (packet_count == 1)
        {
            // file doesn't exist
            option = "w";
        }
        printf("file path %s, option %s\n", filePath, option);
        fp = fopen(filePath, option);

        if (!fp)
        {
            // printf("Unable to open file.\n");
            perror("Unable to open file");
            return 1;
        }
        printf("Opened file at %s\n", filePath);
        printf("recv msg size %d\n", sizeof(recv_msg));
        if (fwrite(recv_msg, 1, sizeof(recv_msg), fp) != sizeof(recv_msg))
        {
            perror("Write to file error");
            exit(1);
        }

        fclose(fp);
        // free(recv_msg);

        // Send Ack
        printf("Start to send Ack\n");
        ackmsg[1] = lastID[0];
        int sendcount = sendto(sockfd, (const char *)ackmsg, sizeof(ackmsg), MSG_CONFIRM, (const struct sockaddr *) &addr, sizeof(addr));
        if (sendcount <= 0)
        {
            printf("Error sending!\n");
            return 1;
        }

        free(recv_buf);
    }


    // }




    return 0;
}
