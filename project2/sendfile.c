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

#define MAX_FILE_SIZE 30000000

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


int main(int argc, char *argv[])
{    
    /* 
     * DGRAM Packet Structure:
     * Packet Type: ACK or Data (1 Bit - 0:Data, 1: Ack)
     * Identifier: Stop and Wait (1 Bit) 
     * Packet Size: 2 Bytes short int
     * Advertised Window: x
     * Directory: 50 Bytes
     * File name: 20 Bytes
     * Data: 1000 Bytes
     * CRC Error Code: 4 Bytes
     */

    // Handle Command Line Inputs
    if (argc!=5)
    {
        printf("Incorrect number of arguments!\n");
        exit(1);
    }
    
    // char* rFlag = argv[1];
    char* host_port = argv[2];
    // char* fFlag = argv[3];
    char* dir_name = argv[4];

    char* ptr1 = strtok(host_port, ":");
    char* hostc = ptr1;
    ptr1 = strtok(NULL, ":");
    char* portc = ptr1;

    char* ptr2 = strtok(dir_name, "/");
    char* dir = ptr2;
    ptr2 = strtok(NULL, "/");
    char* fileName = ptr2;

    unsigned short port = atoi(portc);
    int sockfd;
    char rcv_buffer[2];

    // unsigned int server_address;
    struct sockaddr_in s_in;
    long packet_size = 1078;
    
    char *file_data;

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    // Create a UDP Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&s_in, 0, sizeof(s_in));
    s_in.sin_family = AF_INET;
    s_in.sin_port = htons(port);
    // s_in.sin_addr.s_addr = htons(inet_addr(hostc));
    s_in.sin_addr.s_addr = inet_addr(hostc);

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv); // Set the timeout value

    // Read the FILE and breakdown to package into PACKETS.
    FILE *fp;
    
    fp = fopen("test.bin", "rb");
    if (!fp)
    {
        printf("Unable to open file.\n");
        return 1;
    }
    printf("File open succeeded.\n");


    file_data = (char *)malloc(packet_size * sizeof(char)); // 
    short bytes_read;
    int send_cnt;
    int tmp_cnt;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    size_t total_bytes = 0;
    char *packet_msg;
    packet_msg = malloc(packet_size);
    char directory[50]; 
    strncpy(directory, dir, 50);
    char name[20];
    strncpy(name, fileName, 20);

    char sentID = 0;
    int total_bytes_sent = 0;

    
    while ((bytes_read = fread(file_data, 1, 1000, fp)) > 0)
    {
        memset(packet_msg, 0, 1078);
        // printf("Read %d bytes, now sending...\n", bytes_read);
        total_bytes += bytes_read;
      
        memset(packet_msg, 0, 1);
        // memcpy(packet_msg+1, sentID, 1);
        memset(packet_msg+1, sentID, 1);

        *(short*)(packet_msg+2) = htons(bytes_read);
        memcpy(packet_msg+4, directory, 50); // Directory information
        memcpy(packet_msg+54, name, 20); // File Name
        memcpy(packet_msg+74, file_data, 1000); // Actual Data

        memset(file_data, 0, 1000);

        // Compute CRC
        unsigned int crc = crc32b(packet_msg);
        memcpy(packet_msg+1074, &crc, 4);



        while (1){
            // send packet
            // recv ack
            //if recvid == sendid
            //      change sendID
            //      break

            send_cnt = 0;
            while (send_cnt < packet_size)
            {
                tmp_cnt = sendto(sockfd, (const char *)packet_msg, packet_size, 0, (const struct sockaddr *) &s_in, sizeof(s_in));
                if(tmp_cnt <= 0){
                    printf("Error sending!\n");
                    return 1;
                }    
                send_cnt += tmp_cnt;
            }
            printf("Packet Sent\n");
            total_bytes_sent += send_cnt;
            int bytes_rcvd = recvfrom(sockfd, rcv_buffer, 2, MSG_WAITALL, (struct sockaddr *)&s_in, &addr_len);
            printf("Bytes Rcvd: %d\n", bytes_rcvd);
            if (bytes_rcvd > 0)
            {
                printf("Ack received!\n");
                char rcvID;
                rcvID = *(rcv_buffer+1);
                // memcpy(rcvID, rcv_buffer+1, 1);
                printf("Rcvd ID: %d\n", rcvID);
                printf("Sent ID: %d\n", sentID);

                if (rcvID == sentID)
                {
                    if (sentID == 1)
                    {
                        sentID = 0;
                    }
                    else
                    {
                        sentID = 1;
                    }
                        
                    printf("Sending NEXT packet now...\n");
                    break;
                }
                else
                {
                    printf("Resending packet since ACK ID different from Sent ID\n");
                }
                 
            }
            else
            {
                perror("Error Receiving Ack");
                printf("Resending packet\n");
                // exit(1);
            }
        }
    }
    printf("completed\n");

    
    // Close socket descriptor and exit.
    close(sockfd);
    return 0;
}