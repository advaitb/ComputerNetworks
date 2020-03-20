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
#include "common.h"
#include <pthread.h>
#include <stdbool.h>

typedef unsigned long long ull;
/* initialize some socket variable */
int sockfd;
struct sockaddr_in client_addr;
/* keep listening for ack and update sliding window */
void *send_ackn();

uint8_t* track_recv_window;

int last_frame_received, last_ack_sent; /*  NOTE THE DIFFERENCE IN THESE VARS */

void regularShift(){
    int sft = 1;
    for (int i = 1; i < WINDOW_SIZE; i++) {
            if (!track_recv_window[i]){
                    break;
            }
            sft += 1;
    }
    for (int j  = 0; j  < WINDOW_SIZE - sft; j++) {
            track_recv_window[j] = track_recv_window[j + sft];
    }
    for (int k = WINDOW_SIZE - sft; k < WINDOW_SIZE; k++) {
            track_recv_window[k] = 0;
    }
    last_frame_received += sft;
    last_ack_sent = last_frame_received + WINDOW_SIZE;
}

int main(int argc, char* argv[]){

        if(argc < 3){
                fprintf(stderr, "Not enough args\n \tArgs are: [FNAME] [PORT]\n");
                exit(EXIT_FAILURE);
        }
        char* fname = argv[1]; /* file to send - max file size will be 30MB */
        FILE *f = fopen(fname,"wb"); /* open file in binary */
        fprintf(stderr, "Save received file as: %s\n",fname);
        int max_buffer_size = MAX_DATA_SIZE * BUFFER_SIZE; /* buffer is a multiple of data size */
        fprintf(stderr,"\tMax buffer size set at: %i\n",max_buffer_size);
        struct sockaddr_in s_in;/* socket variables */
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){ /* create UDP  socket */
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
        }

        memset(&s_in, 0, sizeof(s_in));
        s_in.sin_family = AF_INET;
        s_in.sin_port = htons(atoi(argv[2]));
        s_in.sin_addr.s_addr = INADDR_ANY;
    	/* bind server socket to the address */
    	if (bind(sockfd, (struct sockaddr *) &s_in, sizeof(s_in)) < 0){
        	perror("Failed to bind socket to address");
        	exit(EXIT_FAILURE);
    	}
        int buff_size; /* check buff size is  */
        char curr_buffer[max_buffer_size];
        uint8_t finish_read = 0;
        int buff_util = 0;
        /* packet vars */
        char packet[MAX_PACK_SIZE];
        char data[MAX_DATA_SIZE];
        char ackn[ACKN_SIZE];
        int packetsz;
        int datasz;
        int seq_num;
        bool packeterror;
        int is_this_the_end; /* hold your breath and count to 10 */
        uint8_t finish_recv = 0;
        while(!finish_recv){ /* start big while loop */
                buff_size = max_buffer_size;
                memset(curr_buffer, 0, buff_size);
                int seq_cnt = (int) max_buffer_size / MAX_DATA_SIZE;
                track_recv_window = (uint8_t*)malloc(sizeof(uint8_t)*WINDOW_SIZE);
                for(int k = 0; k < WINDOW_SIZE; k++ ){
                        track_recv_window[k]=0;
                }
                last_frame_received = -1; /* none to begin with */
                last_ack_sent = last_frame_received + WINDOW_SIZE;
                while(1){
                        socklen_t client_addr_size;
                        packetsz = recvfrom(sockfd, (char *) packet, MAX_PACK_SIZE, MSG_WAITALL, (struct sockaddr *) &client_addr,  &client_addr_size);
                        is_this_the_end = packet[0];
                        uint32_t pack_seq_num;
                        memcpy(&pack_seq_num, packet + 1, 4);
                        uint32_t pack_data_size;
                        memcpy(&pack_data_size, packet + 5, 4);
                        datasz = ntohl(pack_data_size);
                        memcpy(data, packet + 9, datasz);
                        packeterror = packet[datasz + 9] != csum(packet, datasz + (int) 9);
                        ackn[0] = packeterror == 0x0;
                        memcpy(ackn + 1, &pack_seq_num, 4);
                        ackn[5] = csum(ackn, ACKN_SIZE - (int) 1);
                        sendto(sockfd, ackn, ACKN_SIZE, 0, (const struct sockaddr *) &client_addr, client_addr_size);
                        seq_num = ntohl(pack_seq_num); /* get seequence num */
                        if (seq_num <= last_ack_sent) {
                                if (!packeterror) {
                                        int buff_sft = seq_num * MAX_DATA_SIZE;
                                        if (seq_num == last_frame_received + 1) { /* perfecto! */
                                                memcpy(curr_buffer + buff_sft, data, datasz);
                                                regularShift();
                                        }
                                        else if (seq_num > last_frame_received + 1) { /* out of order dammit! - deal by shift*/
                                                if (!track_recv_window[seq_num - (last_frame_received + 1)]) {
                                                        memcpy(curr_buffer + buff_sft, data, datasz);
                                                        track_recv_window[seq_num - (last_frame_received + 1)] = 1;
                                                }
                                        }
                                        if (is_this_the_end) {
                                                buff_size = buff_sft + datasz;
                                                seq_cnt = seq_num + 1;
                                                finish_recv = 1;
                                        }
                                }
                        }
                        if (last_frame_received >= seq_cnt - 1){ /* We have a contiguous block of packets */
                                break;
                        }
                }

                fwrite(curr_buffer,1,buff_size,f);
                ull received_bytes = (ull) buff_util * (ull) max_buffer_size + (ull) buff_size;
                fprintf(stderr, "\rReceived %llu bytes",received_bytes);
                buff_util = buff_util + 1;
        }
        pthread_t thread;
        pthread_create(&thread, NULL, send_ackn, NULL);
        fclose(f);
        free(track_recv_window); /* free that */
        pthread_detach(thread);
        close(sockfd); /* close socket */
        return 0;
}


void *send_ackn() {
        /* not sure about static linking in C - assuming that variables in header have been defined and are in use by sfile */
        /* need new declarations here ! */
        char packet[MAX_PACK_SIZE];
        char data[MAX_DATA_SIZE];
        char ackn[ACKN_SIZE];
        int packetsz;
        int datasz;
        socklen_t client_addr_size;
        int recpack_seq_num;
        bool packeterror;
        int is_this_the_end;

        /* Listen for frames and send ack */
        while (1) {
                packetsz = recvfrom(sockfd, (char *)packet, MAX_PACK_SIZE, MSG_WAITALL, (struct sockaddr *) &client_addr, &client_addr_size);
                is_this_the_end = packet[0];
                uint32_t pack_seq_num;
                memcpy(&pack_seq_num, packet+1, 4);
                uint32_t pack_data_size;
                memcpy(&pack_data_size, packet + 5, 4);
                datasz = ntohl(pack_data_size);
                memcpy(data, packet + 9, datasz);
                packeterror = (packet[datasz + 9] != csum(packet, datasz + (int) 9));
                ackn[0] = packeterror == 0x0;
                memcpy(ackn + 1, &pack_seq_num, 4);
                ackn[5] = csum(ackn, ACKN_SIZE - (int) 1);
                sendto(sockfd, ackn, ACKN_SIZE, 0, (const struct sockaddr *) &client_addr, client_addr_size);
        }
}

