#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


int main( int argc, char* argv[]){
	/* vars to store command line arguments */
	char* hostname;
	const char* clear_servers[4] =  
	{"ring.clear.rice.edu", "sky.clear.rice.edu", "glass.clear.rice.edu", "water.clear.rice.edu"};
	unsigned short port, size, count;
	/* need 5 arguments */
	if(argc !=  5){
		printf("Not enough arguments!\n");
		exit(1);
	}
	hostname = argv[1];
	port = atoi(argv[2]);
	size = atoi(argv[3]);
	count = atoi(argv[4]);
	/* check for correctness of command line arguments */
	// uint8_t i;
	uint8_t cnt = 0;
	uint8_t i;
	for( i=0; i < 4; i++){
		if(strcmp(hostname,clear_servers[i]) == 0){
			printf("Hostname verfied to be: %s\n",hostname);
			break;
		}
		else{
			cnt++;
		}
	}
	// if(cnt == 4){
	// 	printf("Error: hostname not verifiable. Please use only the allowed hostname in the list\n");
	// 	exit(1);
	// }
	// if(port < 18000 || port > 18200){
	// 	printf("Error: port number should be within 18000 and 18200 but received %d\n", port);
	// 	exit(1);
	// }
	// /*else */if(size < 10 || size > 65535){
	// 	printf("Error: message size should be within 10 and 65535 but received %d\n", size);
	// 	exit(1);
	// }
	/*else*/ if( count < 1 || count > 10000){
		printf("Error: count should be within 1 and 10000 but received %d\n", count);
		exit(1);
	}
	else{
		printf("Params checked [OK]\n");
	}
	/* the message we are sending across */
	// const char* msg = "Hello, this is client";
	char *msg = (char*) malloc(count);	
	char *increment_msg = "0";

	/* establish connection based on sample code provided in class */	
	
	/* our client socket */
  	int sock;
  	/* variables for identifying the server */
  	unsigned int server_addr;
  	struct sockaddr_in sin;
  	struct addrinfo *getaddrinfo_result, hints;
  	/* convert server domain name to IP address */
  	memset(&hints, 0, sizeof(struct addrinfo));
	/* indicates we want IPv4 */	
  	hints.ai_family = AF_INET; 
	/* make buffers to send and recieve data, these buffers are bounded by user provided size */
	char* receive_buff;
	char* send_buff;
	/* intitialize mem to size -  10 bytes */
    int BUF_LEN = 11000;
	receive_buff = (char*) malloc(BUF_LEN);
	send_buff = (char*) malloc(BUF_LEN);	
	/* check if allocated */
	if((!receive_buff) || (!send_buff)){
		perror("Error allocating one/both the buffers");
		abort();	
	}
	/* get server address */
	if (getaddrinfo(hostname, NULL, &hints, &getaddrinfo_result) == 0) {
    		server_addr = (unsigned int) ((struct sockaddr_in *) (getaddrinfo_result->ai_addr))->sin_addr.s_addr;
    		freeaddrinfo(getaddrinfo_result);
  	}	
	/* create socket */
  	if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    	{
      		perror ("Error opening TCP socket");
      		abort();
    	}
	/* fill in the server's address */
  	memset (&sin, 0, sizeof (sin));
  	sin.sin_family = AF_INET;
  	sin.sin_addr.s_addr = server_addr;
  	sin.sin_port = htons(port);
	/* finally time to connect */
	printf("address: %s\n", inet_ntoa(sin.sin_addr));
	if (connect(sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    	{	
      		perror("Error connection to server failed");
      		abort();
    	}
	/* get time of day */
	struct timeval tv;
	/* 2D array to store timing, count - iteration, 0 - sec diff, 1 - usec  diff */ 
	float timings[10][count];
	/* send message count number of times */
	unsigned short m  = 0;
	size = 11;
	uint8_t k;
	for ( k = 0; k < 10; k++)
	{
		while (m < count){
			strcpy(msg+i, increment_msg);
			/* variables to save sec, usec. Should take 4 bytes each */
			// int stv_sec,stv_usec,rtv_sec,rtv_usec;
			int stv_sec,stv_usec,tv_sec,tv_usec;
			if(gettimeofday(&tv,NULL) == 0){
				stv_sec = (int)tv.tv_sec;
				stv_usec = (int)tv.tv_usec;
	 		}
			/* Assign bytes accordingly */
			// *(char*)(send_buff) = (char) size;
			*(short*)(send_buff) = htons(size);
	 		*(int*)(send_buff+2) = htonl(stv_sec);
	 		*(int*)(send_buff+6) = htonl(stv_usec);
			strcpy(send_buff+10, msg);

			/* send message to server */
			printf("\nsend message\n");
			send(sock,send_buff,size,0);

			/* receive message from server */
	        int recv_cnt = recv(sock, receive_buff, BUF_LEN, 0);
			int rsize = (int) ntohs(*(int *)(receive_buff));
			printf("rsize is %d\n", rsize);
			printf("receive count is %d\n", recv_cnt);
			printf("message is %s\n", receive_buff+10);

	        while (rsize != recv_cnt)
	        {
	        	printf("Still transmitting, re-try receiving\n");
		        recv_cnt = recv(sock, receive_buff, BUF_LEN, 0);
				rsize = (int) ntohs(*(int *)(receive_buff));
				printf("rsize is %d\n", rsize);
				printf("receive count is %d\n", recv_cnt);
				printf("message is %s\n", receive_buff+10);
	        }

			/* couldn't receive */
			if (recv_cnt < 0){
				perror("Error receiving failure");
				abort();
			}
			/* retrieve the bytes from the server */
			// rtv_sec = (int) ntohl(*(int *)(receive_buff+2));
			// rtv_usec = (int) ntohl(*(int *)(receive_buff+6));
			
			/* Get current time */
			if(gettimeofday(&tv,NULL) == 0){
				tv_sec = (int)tv.tv_sec;
				tv_usec = (int)tv.tv_usec;
	 		}
			/* calculate latency in millisecs */ 
			float sec_diff = (tv_sec - stv_sec)*1000;
			// float usec_diff = (tv_sec - stv_sec)/1000;	
			float usec_diff = (tv_usec - stv_usec);	
			/* note latency */
			timings[k][m] = sec_diff+usec_diff;
			printf("stv_sec %d, stv_usec %d\n", stv_sec, stv_usec);
			printf("tv_sec %d, tv_usec %d\n", tv_sec, tv_usec);
			printf("Latency observed in iteration %i is %.3f\n",m,timings[k][m]);
			/* increment message size*/
			printf("message: %s\n", msg);
			m++;
			size++;
		}
		m = 0;
		size = 11;
	}
	printf("close connection\n");
	/* close connection and free memory */
	close(sock);
	free(send_buff);
	free(receive_buff);

	/* Write timings into file */
	printf("write timings\n");
	FILE *fp = malloc(10000);

    fp = fopen("./test.txt", "w");
    int c;
    unsigned short j;
    for (c = 0; c < 10; c++)
    {
	    for ( j = 0; j < count; j++)
	    {
	    	fprintf(fp, "%.3f,", timings[c][j]);
	    }
	    fprintf(fp, "\n");
	}
    fclose(fp);
	/* return */
	return 0;

}
	
