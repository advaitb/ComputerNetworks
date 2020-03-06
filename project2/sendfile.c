#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char const *argv[])
{    
    /* DGRAM Packet Structure:
     * Packet Type:  
     * Ack Number:
     * Advertised Window
     * Data
     */
    unsigned short port;
    int sockfd;
    char rcv_buffer[1024];
    char *msg;
    unsigned int server_address;
    struct sockaddr_in s_in;

    // 1. Create a UDP Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&s_in, 0, sizeof(s_in));
    s_in.sin_family = AF_INET;
    s_in.sin_port = htons(port);
    s_in.sin_addr.s_addr = server_address;
    

    // 2. Send Message to Server
    int send_cnt = sendto(sockfd, (const char *)msg, strlen(msg), MSG_CONFIRM, (const struct sockaddr *) &s_in, sizeof(s_in));
    if (send_cnt < 0)
        perror("Datagram could not be sent!");

    printf("Datagram Sent.\n");

    // 3. Wait until ACK from server is received
    int n, len;
    n = recvfrom(sockfd, (const char *)rcv_buffer, sizeof(rcv_buffer), MSG_WAITALL, (struct sockaddr *)&s_in, len);

    // 4. Process reply, and return to Step 2

    // 5. Close socket descriptor and exit.

    return 0;
}
