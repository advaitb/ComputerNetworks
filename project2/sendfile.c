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
     * Packet Type: ACK or Data (1 Bit)
     * Identifier: Stop and Wait (1 Bit) 
     * Advertised Window: x
     * Data: 1000 Bytes
     * CRC Error Code: 4 Bytes
     */

    /* TODO:
     * Add Stop and Wait Ack Scheme -> Replace with Sliding Window Later
     * Error Checking Codes -> Cyclic Redundancy Check
     */

    unsigned short port;
    int sockfd;
    char rcv_buffer[1024];
    char *msg;
    unsigned int server_address;
    struct sockaddr_in s_in;
    

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
    s_in.sin_addr.s_addr = server_address;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv); // Set the timeout value



    // Read the FILE and breakdown to package into PACKETS.
    int packet_size;
    FILE *fp;
    unsigned char buf[MAX_FILE_SIZE];
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
    file_data = (char *)malloc(lSize* sizeof(char));
              
    if (file_data)
    {
        size_t result = fread(file_data, 1, lSize, fp);
        if (result!=lSize)
            perror("Error reading file");
            return 1;
    }







    // Send Message to Server
    int send_cnt = 0;

    // While ALL packets are not sent
    while (1)
    {
        // While EACH packet is not sent
        while (send_cnt != packet_size)
        {
            int tmp_cnt = sendto(sockfd, (const char *)msg, strlen(msg), NULL, (const struct sockaddr *) &s_in, sizeof(s_in));
            send_cnt += tmp_cnt;
        }

        // Wait until you hear ACK for last sent package
        int len;
        recvfrom(sockfd, (const char *)rcv_buffer, sizeof(rcv_buffer), MSG_WAITALL, (struct sockaddr *)&s_in, len); // If ACK is not recvd it will timeout!
        // Check if this ack is for Packet
        
        
        
    }
    
    

    // 4. Process reply, and return to Step 2

    // 5. Close socket descriptor and exit.

    return 0;
}
