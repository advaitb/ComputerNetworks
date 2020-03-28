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

unsigned int crc32b(char *message, long msg_len) {
    int i, j;
    unsigned int byte, crc, mask;

    i = 0;
    crc = 0xFFFFFFFF;
    while (i < msg_len) {
        if (message[i] == 0)
        {
            i = i + 1;
            continue;
        }
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
    
    char* port; //= argv[2]; 
    int opt; 
    for(opt = 1; opt < argc; opt++){
	if(argv[opt][0] == '-'){
		switch(argv[opt][1]){
			case 'p':
				if((argv[opt+1][0] != '-') && (argv[opt+1][0] != ' ')) port = argv[opt+1];
				else{
					 fprintf(stderr, "Missing port\n");
					 exit(EXIT_FAILURE);
				}
				break;
			default:
				fprintf(stderr, "Invalid option\n");
				exit(EXIT_FAILURE);
		}
	}
     }	
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
    tv.tv_sec = 100;
    tv.tv_usec = 0;
    
    int DATA_SIZE = 25000;
    int HEADER_SIZE = 75;
    int CRC_SIZE = 4;
    long packet_size = DATA_SIZE + HEADER_SIZE + CRC_SIZE;
    short ack_size = 4;

    char* recv_buf;
    char ackmsg[3];
    char* ack_packet;
    ack_packet = malloc(ack_size);
    char dir[50];
    char fileName[20];
    short msg_size;
    int packet_count = 0;
    FILE *fp;

    ackmsg[0] = 1;
    short lastID;
    lastID = -1;
    int total_data = 0;

    // Receive all the packets
    while(1)
    {
        recv_buf = (char *)malloc(packet_size);
        int count = recvfrom(sockfd, recv_buf, packet_size, MSG_WAITALL, (struct sockaddr *)&addr, &addr_len);
        if (count < 0)
            break;

        // Check if crc32b's are the same
        unsigned int crc = crc32b(recv_buf, packet_size - CRC_SIZE);
        unsigned int crc_send = *(unsigned int *)(recv_buf + packet_size - CRC_SIZE);
        if (crc != crc_send)
        {
            printf("[recv corrupt packet]\n");
            free(recv_buf);

            // In this case an ACK is still sent
            *(short*) (ackmsg+1) = htons(lastID);
            char csum_ack = csum(ackmsg, 3);
            memcpy(ack_packet, ackmsg, 3);
            memcpy(ack_packet+3, &csum_ack, 1);
            int sendcount = sendto(sockfd, (const char *)ack_packet, ack_size, MSG_CONFIRM, (const struct sockaddr *) &addr, sizeof(addr));
            if (sendcount <= 0)
            {
                printf("Error sending!\n");
                return 1;
            }
            continue; // If error detected, discard the packet
        }
        
        // Check sequence number in stop & wait fashion
        short recvID;
        recvID = (short) ntohs(*(short *)(recv_buf+1));
        msg_size = (short) ntohs(*(short *)(recv_buf+3));
        
        if (recvID == lastID+1)
        {
            lastID = recvID;
            total_data += msg_size;
            printf("[recv data] %d %u ACCEPTED\n", (total_data - msg_size), msg_size);
            
        }
        else
        {
            free(recv_buf);
            printf("[recv data] %d %u IGNORED\n", (total_data - msg_size), msg_size);

            // In this case an ACK is still sent
            *(short*) (ackmsg+1) = htons(lastID);
            char csum_ack = csum(ackmsg, 3);
            memcpy(ack_packet, ackmsg, 3);
            memcpy(ack_packet+3, &csum_ack, 1);
            int sendcount = sendto(sockfd, (const char *)ack_packet, ack_size, MSG_CONFIRM, (const struct sockaddr *) &addr, sizeof(addr));
            if (sendcount <= 0)
            {
                printf("Error sending!\n");
                return 1;
            }
            continue;
        }

        packet_count += 1;
        if (packet_count == 1)
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv); // Set the timeout value

        // Read data from the packet
        memcpy(dir, recv_buf+5, 50);

        char recv_msg[msg_size];
        memcpy(fileName, recv_buf+55, 20);
        memcpy(recv_msg, recv_buf+75, msg_size);

        // Save to the file
        char filePath[70];
        strcpy(filePath, dir);
        strcat(filePath, "/");
        strcat(filePath, fileName);
        strcat(filePath, ".recv");
        char* option = "a";
        if (packet_count == 1)
        {
            // file doesn't exist
            option = "w";
        }
        fp = fopen(filePath, option);

        if (!fp)
        {
            perror("Unable to open file");
            return 1;
        }
        if (fwrite(recv_msg, 1, sizeof(recv_msg), fp) != sizeof(recv_msg))
        {
            perror("Write to file error");
            exit(1);
        }
        fclose(fp);

        // Send Ack
        *(short*) (ackmsg+1) = htons(lastID);
        char csum_ack = csum(ackmsg, 3);
        memcpy(ack_packet, ackmsg, 3);
        memcpy(ack_packet+3, &csum_ack, 1);
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
