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

int main(int argc, char const *argv[])
{    
    /* 
     * DGRAM Packet Structure:
     * Packet Type: ACK or Data (1 Bit - 0:Data, 1: Ack)
     * Identifier: Stop and Wait (1 Bit) 
     * Advertised Window: x
     * Directory: 50 Bytes
     * File name: 20 Bytes
     * Data: 1000 Bytes
     * CRC Error Code: 4 Bytes
     */

    /* TODO:
     * Add Stop and Wait Ack Scheme -> Replace with Sliding Window Later
     * Error Checking Codes -> Cyclic Redundancy Check
     */

    // Handle Command Line Inputs
    if (argc!=5)
    {
        printf("Incorrect number of arguments!\n");
        exit(1);
    }
    
    char* rFlag = argv[1];
    char* host_port = argv[2];
    char* fFlag = argv[3];
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
    char rcv_buffer[1024];
    char *msg;
    // unsigned int server_address;
    struct sockaddr_in s_in;
    long packet_size = 1076;
    

    long lSize;
    char *file_data;

    struct timeval tv;
    tv.tv_sec = 1;
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
    fseek(fp, 0, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    file_data = (char *)malloc(packet_size * sizeof(char)); // 
    size_t bytes_read;
    int send_cnt;
    int tmp_cnt;
    int addr_len;
    size_t total_bytes = 0;
    char *packet_msg;
    packet_msg = malloc(packet_size);
    char* directory[50]; 
    strncpy(directory, dir, 50);
    char* name[20];
    strncpy(name, fileName, 20);

    while ((bytes_read = fread(file_data, 1, packet_size, fp)) > 0)
    {
        // printf("Read %d bytes, now sending...\n", bytes_read);
        total_bytes += bytes_read;

        // construct packet message
        packet_msg[0] = 0; // Data Message
        packet_msg[1] = 0; // Stop and Wait Scheme
        strcpy(packet_msg+2, directory); // Directory information
        strcpy(packet_msg+52, name);
        strcpy(packet_msg+72, file_data);

        char* CRC_code[4];
        strcpy(packet_msg+1072, CRC_code);
        
        send_cnt = 0;
        // while (send_cnt != packet_size)
        // {
        //     tmp_cnt = sendto(sockfd, (const char *)packet_msg, strlen(packet_msg), NULL, (const struct sockaddr *) &s_in, sizeof(s_in));
        //     send_cnt += tmp_cnt;
        // }
        // recvfrom(sockfd, (const char *)rcv_buffer, sizeof(rcv_buffer), MSG_WAITALL, (struct sockaddr *)&s_in, addr_len);
    }
    printf("Complete file sent. %d bytes sent.\n", total_bytes);
    
    
    // Close socket descriptor and exit.
    return 0;
}
