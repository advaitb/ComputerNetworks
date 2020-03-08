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

    unsigned short server_port;
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
    if (bind(sockfd, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
        perror("binding socket to address");
        abort();
    }

    // Put the socket in listen mode
    if (listen(sockfd, BACKLOG) < 0)
    {
        perror("listen on socket failed");
        abort();
    }

    // Keep waiting for incoming connection
    while(1)
    {
        new_sock = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
        if (new_sock < 0)
        {
            perror("error accepting connection");
            abort();
        }
    }




        return 0;
}
