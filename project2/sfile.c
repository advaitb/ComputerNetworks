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
struct sockaddr_in server_addr;
/* keep listening for ack and update sliding window */
uint8_t* track_ack_window; /* check whether ack has been received */
uint8_t* track_sent_window; /* check whether packet in the window has been sent */
time_t* track_sent_time; /* check for timeout ? */
void *check_ackn();
/* need to lock sliding window */
pthread_mutex_t slock;

//time_t init_time;
//init_time  = time(NULL);

#define INIT_TIME time(NULL)

char packet[MAX_PACK_SIZE]; /* hold packet */
char data[MAX_DATA_SIZE]; /* data part of the packet */
int packetsz; /* packet size */
int datasz; /* data size */

/* get current time */
time_t getCurrentTime(){
        time_t curr_time;
        curr_time = time(NULL);
        return curr_time;
}

/* get time elapsed */
time_t timeElapsed(time_t curr_time, time_t init_time){
        return curr_time-init_time;
}

int last_ack_received, last_frame_sent; /* variables track whether to slide window */

void setWindowBits(){
    if (track_ack_window[0]){
        int sft = 1;
        for (int i = 1; i < WINDOW_SIZE; i++) {
            if(!track_ack_window[i]){
                    break;
            }
            sft += 1;
        }
        for (int j = 0; j < WINDOW_SIZE - sft; j++) {
            track_sent_time[j] = track_sent_time[j + sft];
            track_sent_window[j] = track_sent_window[j + sft];
            track_ack_window[j] = track_ack_window[j + sft];
        }
       for (int k = WINDOW_SIZE - sft; k < WINDOW_SIZE; k++) {
            track_ack_window[k] = 0;
            track_sent_window[k] = 0;
        }
        last_ack_received += sft;
        last_frame_sent = last_ack_received + WINDOW_SIZE;
    }
}

int main(int argc, char* argv[]){
        if(argc < 4){
                fprintf(stderr,"Not enough arguments\n \tArgs are: [FNAME] [SERVER_IP] [PORT]\n");
                exit(EXIT_FAILURE);
        }
        if (pthread_mutex_init(&slock, NULL) != 0) {
                printf("\n mutex init has failed\n");
                return 1;
        }
        //int port; /* socket variables */
        //unsigned int server_address;/* socket variables */
        struct sockaddr_in s_in;/* socket variables */ 
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){ /* create UDP  socket */
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
        }
        
        memset(&s_in, 0, sizeof(s_in));
        s_in.sin_family = AF_INET;
        s_in.sin_port = htons(atoi(argv[3]));
        s_in.sin_addr.s_addr = inet_addr(argv[2]);
        char* fname = argv[1]; /* file to send - max file size will be 30MB */
        FILE *f = fopen(fname,"rb"); /* open file in binary */
        fprintf(stderr, "File to send: %s\n",fname);
        int max_buffer_size = MAX_DATA_SIZE * BUFFER_SIZE; /* buffer is a multiple of data size */
        fprintf(stderr,"\tMax buffer size set at: %i\n",max_buffer_size); 
        int buff_size; /* check buff size is  */
        char curr_buffer[max_buffer_size];
        uint8_t finish_read = 0;
        int buff_util = 0;
        /* read from file */
        pthread_t thread; /* init the thread */
        pthread_create(&thread, NULL, check_ackn, NULL ); /* start running thread */
        while(!finish_read){
                buff_size  = fread(curr_buffer, 1, max_buffer_size, f);
                //fprintf(stderr,"Buffer has: %s",curr_buffer);
                if(buff_size  ==  max_buffer_size){
                        char check_one_byte_ahead[1];
                        if (fread(check_one_byte_ahead,1,1,f) == 0 ){
                                finish_read  = 1; /* we have reached EOF */
                        }
                        fseek(f,-1,SEEK_CUR); /* reset cursor in any case, don't want to lose data */
                }
                else{
                        finish_read = 1;
                }
                pthread_mutex_lock(&slock); /* access new sliding window */  
                last_ack_received = -1;
                last_frame_sent = last_ack_received + WINDOW_SIZE;
                track_ack_window = (uint8_t*)malloc(sizeof(uint8_t)*WINDOW_SIZE); /* initialize ack window tracker */
                track_sent_window = (uint8_t*)malloc(sizeof(uint8_t)*WINDOW_SIZE); /* initialize sent window tracker */
                track_sent_time = (time_t*)malloc(sizeof(time_t)*WINDOW_SIZE);/* initialize sent time tracker */
                /* begin new sliding window */
                int seq_cnt = buff_size / MAX_DATA_SIZE + ((buff_size % MAX_DATA_SIZE == 0) ? 0 : 1);
                int seq_num;
                for (int i = 0; i < WINDOW_SIZE; i++) { /* set everything to false while  initialization */
                    track_ack_window[i] = 0;
                    track_sent_window[i] = 0;
                }
                pthread_mutex_unlock(&slock); /* unlock */
                /* check how much of the data is sent */
                uint8_t sent_all = 0; /*  sent all set to false initially */
                while (!sent_all){
                    pthread_mutex_lock(&slock); /* lock it */
                    setWindowBits();
                    pthread_mutex_unlock(&slock); /* unlock */
                    for (int i = 0; i < WINDOW_SIZE; i ++) {
                        seq_num = last_ack_received + i + 1;
                        if (seq_num < seq_cnt){
                            pthread_mutex_lock(&slock);
                            if (!track_sent_window[i] || (!track_ack_window[i] && (timeElapsed(getCurrentTime(), track_sent_time[i]) > TIMEOUT))) {
                                int buff_sft = seq_num * MAX_DATA_SIZE;
                                if (buff_size - buff_sft < MAX_DATA_SIZE){
                                        datasz = (buff_size - buff_sft);
                                }
                                else{
                                        datasz = MAX_DATA_SIZE;
                                }
                                memcpy(data, curr_buffer + buff_sft, datasz);
                                uint32_t pack_datasz = htonl(datasz);
                                uint32_t pack_seq_num  = htonl(seq_num);
                                if ((seq_num == seq_cnt - 1) && (finish_read)){
                                        packet[0] = 0x0;
                                }
                                else{
                                        packet[0] = 0x1;
                                }
                                
                                memcpy(packet+1,&pack_seq_num,4);
                                memcpy(packet+5,&pack_datasz,9);
                                memcpy(packet+9,data,datasz);
                                packet[datasz + 9] = csum(packet, datasz + (int) 9);
                                packetsz = datasz + (int) 9;
                                sendto(sockfd, packet, packetsz, 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
                                track_sent_window[i] = 1;
                                track_sent_time[i] = getCurrentTime();
                            }
                            pthread_mutex_unlock(&slock);
                        }
                    }
                    if (last_ack_received >= seq_cnt - 1){
                            sent_all = 1;
                    }
                }
                ull data_sent = (ull) buff_util  * (ull) max_buffer_size  + (ull) buff_size;
                fprintf(stderr,"\rSending packets worth %llu",data_sent);
                buff_util += 1;
                if(finish_read){
                        break; /*  big while loop ends */
                }
        
        free(track_ack_window); /* free 'em dyn vars */
        free(track_sent_window);
        free(track_sent_time);
	
	}
        fclose(f);
        pthread_mutex_destroy(&slock);
        pthread_detach(thread);
        close(sockfd); /* close socket */
        return 0;
}


void *check_ackn(){

        char ackn[ACKN_SIZE];
        int acknsz, ackn_seq_num;
        bool acknerror,ret_neg;
        while(1){
                socklen_t server_addr_size;
                acknsz = recvfrom(sockfd, (char *)ackn, ACKN_SIZE, MSG_WAITALL, (struct sockaddr *) &server_addr, &server_addr_size);/* recv ackn */
                fprintf(stderr,"does this work?");
                ret_neg  = ackn[0] == 0x0;
                uint32_t pack_seq_num;
                memcpy(&pack_seq_num, ackn + 1, 4);
                ackn_seq_num = ntohl(pack_seq_num);
                acknerror = (ackn[5] != csum(ackn, ACKN_SIZE - (int) 1));
                pthread_mutex_lock(&slock);
                if (!acknerror && ackn_seq_num > last_ack_received && ackn_seq_num <= last_frame_sent) {
                    if (!ret_neg) {
                        track_ack_window[ackn_seq_num - (last_ack_received + 1)] = 1;
                    } else {
                        track_sent_time[ackn_seq_num - (last_ack_received + 1)] = INIT_TIME;
                    }
                }
                pthread_mutex_unlock(&slock);
        }
}
