#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


int main( int argc, char* argv[]){

	/*
 	* vars to store command line arguments
 	*/
	char* hostname;
	char* clear_servers[4] = {"ring.clear.rice.edu", "sky.clear.rice.edu", "glass.clear.rice.edu", "water.clear.rice.edu"};
	uint32_t port, size, count;
	
	/*
	 *need 5 arguments
	 */
	if(argc !=  5){
		printf("Not enough arguments!\n");
		exit(1);
	}

	hostname = argv[1];
	port = atoi(argv[2]);
	size = atoi(argv[3]);
	count = atoi(argv[4]);

	/*
	 *check for correctness of commandline arguments
	 */
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

	/*
	 * establish connection
	 */	
	
	


	return 0;

}

