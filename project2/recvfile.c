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
    int sockfd, new_sock;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Maximum number of pending connection requests
    int BACKLOG = 5;


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
        perror("binding socket to address");
        exit(EXIT_FAILURE);
    }

    // Put the socket in listen mode
    if (listen(sockfd, BACKLOG) < 0)
    {
        perror("listen on socket failed");
        exit(EXIT_FAILURE);
    }

    long packet_size = 1076;
    long HEADER_LEN = 76;

    char* recv_buf;
    char ackmsg[2];
    char dir[50];
    char fileName[20];
    char recv_msg[packet_size - HEADER_LEN];

    // Keep waiting for incoming connection
    while(1)
    {
        // Accept a new socket
        new_sock = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
        if (new_sock < 0)
        {
            perror("error accepting connection");
            abort();
        }

        // Connection is made
        printf("Accepted connection. Client IP address: %s\n", inet_ntoa(addr.sin_addr));

        FILE *fp;

        ackmsg[0] = 1;

        char lastID[1];
        lastID[0] = 1;
        // Receive all the packets
        while(1)
        {
            recv_buf = (char *)malloc(packet_size);
            int count = recvfrom(new_sock, recv_buf, sizeof(recv_buf), MSG_WAITALL, (struct sockaddr *)&addr, &addr_len);
            int tempcount = 0;
            while (count < packet_size)
            {
                tempcount = recvfrom(new_sock, recv_buf+count, sizeof(recv_buf)-count, MSG_WAITALL, (struct sockaddr *)&addr, &addr_len);
                if (tempcount == -1)
                    continue;
                count += tempcount;
            }
            // Check crc
            unsigned int crc = crc32b(recv_buf);
            if (crc != 0)
            {
                free(recv_buf);
                continue; // If error detected, discard the packet
            }

            // Check sequence number in stop & wait fashion
            char recvID = recv_buf[1];
            if (recvID != lastID[0])
                lastID[0] = recvID;
            else
            {
                free(recv_buf);
                continue;
            }

            // Copy the packet to the message
            // memcpy(recv_msg, &recv_buf, packet_size);
            strncpy(dir, recv_buf+2, 50);
            strncpy(fileName, recv_buf+52, 20);
            strncpy(recv_msg, recv_buf+72, 1000);

            // Save message to the file
            char filePath[70];
            strcpy(filePath, dir);
            strcat(filePath, fileName);
            fp = fopen(filePath, "a");
            if (!fp)
            {
                printf("Unable to open file.\n");
                return 1;
            }
            if (fwrite(recv_msg, 1, sizeof(recv_msg), fp) != 1)
            {
                printf("Write to file error!\n");
                exit(1);
            }

            fclose(fp);
            free(recv_msg);

            // Send Ack
            ackmsg[1] = lastID[0];
            int sendcount = sendto(new_sock, (const char *)ackmsg, sizeof(ackmsg), 0, (const struct sockaddr *) &addr, sizeof(addr));
            if (sendcount <= 0)
            {
                printf("Error sending!\n");
                return 1;
            }

            free(recv_buf);
        }


    }




    return 0;
}
