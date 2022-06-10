/*
*	Title: 
*		CSE5462-Lab4: client.c
*	Author: 
*		Jason Xu (xu.2460)
*	Description:
*		This program integrates a DGRAM client and server together
*	Credit:
*		Dr. Dave Ogle 
*/

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>

#define BUFFER_LEN 29
#define MAX_ENTRY 30
#define MSG_LEN 100 // 100 bytes + 6 header bytes
#define STDIN 0
#define VERSION 1

int readConfigFile(char* ipAddress[], int* portNumber[], int* location[]);
int isValidIpAddr (char* ipaddr);
void printInputErrorMsg(char* msg);

int main (int argc, char *argv[])
{	
	if(argc != 2) printInputErrorMsg("Error: Usage is: client <portNumber>\n");
	
	/* Read entries from config.txt */
	int i;
	char* ipAddress[MAX_ENTRY];
	int* portNumber[MAX_ENTRY];
	int* location[MAX_ENTRY];
	int numHosts = readConfigFile(ipAddress, portNumber, location);
	printf("=======================\n");
	printf("Reading from config.txt\n");
	printf("=======================\n");
	for (i = 0; i < numHosts; i++){
		printf("Location %d @ <%s:%d>\n", *location[i], ipAddress[i], *portNumber[i]);
	}
	printf("\n");
	
	/* Read <portnumber> from command line and check input validality */
	int loc = -1;
	int locIdx = -1;
	char* currIpAddr;
	int currPortNum;
	int port;
	port = strtol(argv[1], NULL, 10);
	for (i = 0; i < numHosts; i++) {
		if (port == *portNumber[i]) {
			loc = *location[i];
			locIdx = i;
			currIpAddr = ipAddress[i];
			currPortNum = *portNumber[i];
		}
	}
	if (loc == -1) printInputErrorMsg("Error: No match of <portNumber> found\n");
		
	/* Variable declaration */
	int sd;
	int rc; // return code for recvfrom
	struct sockaddr_in server_address;
	struct sockaddr_in from_address;
	socklen_t fromLength = sizeof(struct sockaddr);
	fd_set sd_set; //socket descriptor set
	int maxSD; //max socket descriptor
	
	/* Creating a socket */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		close(sd);
		exit(-1);
	}
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;
	
	/* Bind: Assign address to the unbound socket */ 
	if(bind(sd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){
		perror("bind");
		close(sd);
		exit(1);
	}
	
	/* Select: STDIN or SD mode */
	char buffer[MSG_LEN+6]; // version + spc + loc + spc
	char msg[MSG_LEN]; 
	int msg_len;
	int recvVer;
	int recvLoc;
	printf("===============================\n");
	printf("My Current Location is %d:\n", loc);
	printf("\t@ <%s:%d>\n", currIpAddr, currPortNum);
	while (1) {
		printf("===============================\n");
		printf("Msg to send:(enter STOP to exit)\n");

		memset(buffer, '\0', sizeof(buffer));
		memset(msg, '\0', sizeof(msg));
		
		FD_ZERO(&sd_set);
		FD_SET(sd, &sd_set);
		FD_SET(STDIN, &sd_set);
		maxSD = (STDIN > sd) ? STDIN : sd;
		rc = select(maxSD+1, &sd_set, NULL, NULL, NULL);
		
		/* Client mode if STDIN detected */
		if (FD_ISSET(STDIN, &sd_set)) {
			fgets(msg, sizeof(msg), stdin);
			msg[strlen(msg) - 1] = '\0'; //Change last char from \n to \0

			if (!strcmp(msg, "STOP")) break;
			
			// buffer = version + " " + location + " " + buffer
			msg_len = strlen(msg);
			sprintf(buffer, "%d %d %s", VERSION, loc, msg);
			printf("[Sending]->\n");
			for (i = 0; i < numHosts; i++){
				if (i == locIdx) continue;
				
				server_address.sin_family = AF_INET;
				server_address.sin_port = htons(*portNumber[i]);
				server_address.sin_addr.s_addr = inet_addr(ipAddress[i]);
				
				rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
				if (rc < 0) {
					close(sd);
					perror("sendto");
					return -1;
				}	
				printf("%d bytes sent to location %d\n\t@ <%s:%d>\n", msg_len, *location[i], ipAddress[i], *portNumber[i]);
			}
		}
		
		/* Server mode if not STDIN */
		if (FD_ISSET(sd, &sd_set)) {
			rc = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_address, &fromLength);
			if(rc < 0){
				close(sd);
				perror("recvfrom");
				return -1;
			}
			sscanf(buffer, "%d %d %[^\n]s", &recvVer, &recvLoc, msg);
			msg_len = strlen(msg);
			printf("[Receiving]<-\n");
			printf("%d bytes of version %d from location %d:\n%s\n", msg_len, recvVer, recvLoc, msg);
		}
		
	}
	close(sd);
	
	printf("[Exiting]..\n");
	return 0;
}

/*
*	This function will read the entry from config.txt and validate the format.
*/
int readConfigFile(char* ipAddress[], int* portNumber[], int* location[]) {	
	/* Writing entry in to pointers */
	int i;
	for (i = 0; i < MAX_ENTRY; i++) {
        if ((ipAddress[i] = malloc(sizeof(char) * BUFFER_LEN)) == NULL) {
            perror("malloc");
            return -1;
        }
        if ((portNumber[i] = malloc(sizeof(int) * BUFFER_LEN)) == NULL) {
            perror("malloc");
            return -1;
        } 
        if ((location[i] = malloc(sizeof(int) * BUFFER_LEN)) == NULL) {
            perror("malloc");
            return -1;
		} 
	}

	FILE *fp;
	char* end;
	char dataBuffer[BUFFER_LEN];
	int count = 0;
	/* Reading from config.txt */
	memset(dataBuffer, '\0', sizeof(dataBuffer));
	fp = fopen("./config.txt", "r");   
	while (fscanf(fp, "%s", dataBuffer) != EOF) {
		int setNum = (int)(count / 3.0);
		if (count % 3 == 0){
			if (!isValidIpAddr(dataBuffer)) printInputErrorMsg("Error: invalid <ipaddr>\nCheck config.txt, Usage is: <ipaddr> <portNumber> <location>\n");
			strcpy(ipAddress[setNum], dataBuffer);			
		} else if (count % 3 == 1) {
			*portNumber[setNum] = strtol(dataBuffer, &end, 10);
			if (end == dataBuffer) printInputErrorMsg("Error: invalid <portNumber>\nCheck config.txt, Usage is: <ipaddr> <portNumber> <location>\n");
		} else {
			*location[setNum] = strtol(dataBuffer, &end, 10);
			if (end == dataBuffer) printInputErrorMsg("Error: invalid <location>\nCheck config.txt, Usage is: <ipaddr> <portNumber> <location>\n");
		}
		count++;
	}
	fclose(fp);
	
	return count / 3;
}

/*
*	This function will check if a string is valid IP address.
*/
int isValidIpAddr(char *ipaddr){
	struct sockaddr_in sockaddr;
	int result = inet_pton(AF_INET, ipaddr, &(sockaddr.sin_addr));
	return result == 1;
}

/*
*	Print error msg from invalid usage of command line and config.txt
*/
void printInputErrorMsg(char* msg){
	printf(msg);
	exit(1);
}


