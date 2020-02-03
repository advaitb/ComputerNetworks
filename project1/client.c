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
	char* clear_servers[4] = {"ring.clear.rice.edu", "sky.clear.rice.edu", "glass.clear.rice.edu", "water.clear.rice.edu"};
	uint32_t port, size, count;
	
	/* need 5 arguments */
	if(argc !=  5){
		printf("Not enough arguments!\n");
		exit(1);
	}

	hostname = argv[1];
	port = atoi(argv[2]);
	size = atoi(argv[3]);
	count = atoi(argv[4]);

	/* check for correctness of commandline arguments */
	uint8_t i;
	uint8_t cnt = 0;
	for(i=0; i < 4; i++){
		if(strcmp(hostname,clear_servers[i]) == 0){
			printf("Hostname verfied to be: %s\n",hostname);
			break;
		}
		else{
			cnt++;
		}
	}
	if(cnt == 4){
		printf("Error: hostname not verifiable. Please use only the allowed hostname in the list\n");
		exit(1);
	}

	if(port < 18000 || port > 18200){
		printf("Error: port number should be within 18000 and 18200 but received %d\n", port);
		exit(1);
	}
	else if(size < 10 || size > 65535){
		printf("Error: message size should be within 10 and 65535 but received %d\n", size);
		exit(1);
	}
	else if( count < 1 || count > 10000){
		printf("Error: count should be within 1 and 10000 but received %d\n", count);
		exit(1);
	}
	else{
		printf("Params checked [OK]\n");
		exit(1);
	}

	/* establish connection based on sample code provided in class */	
	
	/* our client socket */
  	int sock;

  	/* variables for identifying the server */
  	unsigned int server_addr;
  	struct sockaddr_in sin;
  	struct addrinfo *getaddrinfo_result, hints;

  	/* convert server domain name to IP address */
  	memset(&hints, 0, sizeof(struct addrinfo));
  	hints.ai_family = AF_INET; /* indicates we want IPv4 */	
	
	/* make buffers to send and recieve data, these buffers are bounded by user provided size */
	char* receive_buff;
	char* send_buff;
	
	/* intitialize mem to size -  10 bytes */
	receive_buff = (char*) malloc(size);
	send_buff = (char*) malloc(size);

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
	if (connect(sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    	{	
      		perror("Error connection to server failed");
      		abort();
    	}

	
	/* get time of day */
	struct timeval tv;
	struct timezone tz;
	/* variables to save sec, usec. Should take 4 bytes each */
	int tv_sec,tv_usec;
	if(gettimeofday(&tv,&tz) == 0){
		int tv_sec = tv.tv_sec;
		int tv_usec = tv.tv_usec;
	}

	/* return */
	return 0;

}
	
