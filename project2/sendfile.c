#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_FILE_SIZE 30000000
#define MSB_MASK 0xFFFF0000
#define LSB_MASK 0xFFFF
//comment
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

time_t getCurrentTime(){
        time_t currtime;
        currtime = time(NULL);
        return currtime;
}

time_t getTimeElapsed(time_t end_time, time_t start_time){
        return (end_time - start_time);
}

time_t sum_time;

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
    char* host_port;
    char* dir_name;
    int opt; 
    for(opt = 1; opt < argc; opt++){
	if(argv[opt][0] == '-'){
		switch(argv[opt][1]){
			case 'r':
				if((argv[opt+1][0] != '-') && (argv[opt+1][0] != ' ') && (strchr(argv[opt+1],':') != NULL)) host_port = argv[opt+1];
				else{
					 fprintf(stderr, "Missing host_ip:port\n");
					 exit(EXIT_FAILURE);
				}
				break;
			case 'f':	
				if((argv[opt+1][0] != '-') && (argv[opt+1][0] != ' ') && (strchr(argv[opt+1],'/')!= NULL)) dir_name = argv[opt+1];
				else{
					 fprintf(stderr, "Missing dir/fname\n");
					 exit(EXIT_FAILURE);
				}
				break;
			default:
				fprintf(stderr, "Invalid option\n");
				exit(EXIT_FAILURE);
		}
	}
     }	

/*
    int opt = 0;
    while((opt = getopt(argc, argv, "r:f:")) != 1){
	    switch(opt){
		    case 'r':
			    host_port = optarg;
			    break;
		    case 'f':
			    dir_name  = optarg;
			    break;
		    case '?':
			        if (optopt == 'r') {
    					printf("\nMissing host_ip and port");
  				} else if (optopt == 'f') {
     					printf("\nMissing dir/filename");
  				} else {	
     					printf("\nInvalid option received");
  				}
  				break;
 			}	
     }	
*/
    //exit(1);
     if (optind >= argc) {
           fprintf(stderr, "Expected argument after options\n");
           exit(EXIT_FAILURE);
     }

    // char* rFlag = argv[1];
    //char* host_port = argv[2];
    // char* fFlag = argv[3];
    
    //char* dir_name = argv[4];
    char fopen_file[sizeof(dir_name)];
    strcpy(fopen_file, dir_name);

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
    short ack_size = 4;
    char rcv_buffer[ack_size];

    struct sockaddr_in s_in;
    
    char *file_data;


    int DATA_SIZE = 25000;
    int HEADER_SIZE = 75;
    int CRC_SIZE = 4;
    long packet_size = DATA_SIZE + HEADER_SIZE + CRC_SIZE;

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

    

    // Read the FILE and breakdown to package into PACKETS.
    FILE *fp;
    fp = fopen(fopen_file, "rb");
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

    short sentID = 0;
    int total_bytes_sent = 0;

    struct timeval tv;
    tv.tv_sec = 0; // initial timeout is 0.1 seconds
    tv.tv_usec = 100000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv); // Set the timeout value
    while ((bytes_read = fread(file_data, 1, DATA_SIZE, fp)) > 0)
    {
        memset(packet_msg, 0, packet_size);
        total_bytes += bytes_read;
      
        memset(packet_msg, 0, 1);
        *(short*)(packet_msg+1) = htons(sentID);
        *(short*)(packet_msg+3) = htons(bytes_read);
        memcpy(packet_msg+5, directory, 50); // Directory information
        memcpy(packet_msg+55, name, 20); // File Name
        memcpy(packet_msg+75, file_data, DATA_SIZE); // Actual Data

        memset(file_data, 0, DATA_SIZE);

        // Compute CRC
        unsigned int crc = crc32b(packet_msg, DATA_SIZE + HEADER_SIZE);
        memcpy(packet_msg + (DATA_SIZE+HEADER_SIZE), &crc, CRC_SIZE);

        while (1){
            // send packet
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
            total_bytes_sent += send_cnt;
            printf("[send data] %ld %d\n", total_bytes-bytes_read, bytes_read);

            // receive ACK
            int bytes_rcvd = recvfrom(sockfd, rcv_buffer, ack_size, MSG_WAITALL, (struct sockaddr *)&s_in, &addr_len);

            if (bytes_rcvd > 0)
            {
                // Check error
                char csum_ack = csum(rcv_buffer, 3);
                if (csum_ack != rcv_buffer[3])
                {
                    printf("[recv corrupt packet]\n");
                    continue; // If error detected, discard the packet
                }

                short rcvID;
                rcvID = (short) ntohs(*(short *)(rcv_buffer+1));

                // Check ACK ID number
                if (rcvID == sentID)
                {
                    sentID += 1;
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
            }
        
        }

    }
    printf("[completed]\n");

    // Close socket descriptor and exit.
    close(sockfd);
    return 0;
}
