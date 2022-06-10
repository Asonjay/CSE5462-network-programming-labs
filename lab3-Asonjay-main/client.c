/*
Title: 
	CSE5462-Lab3: client3.c
Author: 
	Jason Xu (xu.2460)
Description:
	This program creates a DGRAM socket client which can send out message
	to different port in config.txt 
Credit:
	Dr. Dave Ogle 
*/

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_LEN 29
#define MAX_ENTRY 30

int readConfigFile(char* ipAddress[], int* portNumber[]);
int isValidIpAddr (char *ipaddr);

int main (int argc, char *argv[])
{
	/* Setting up buffer and array variables */
	char* ipAddress[MAX_ENTRY];
	int* portNumber[MAX_ENTRY];
	int i;
	for (i = 0; i < MAX_ENTRY; i++) {
        if ((ipAddress[i] = malloc(sizeof(char) * BUFFER_LEN)) == NULL) {
            printf("unable to allocate memory\n");
            return -1;
        }
        if ((portNumber[i] = malloc(sizeof(int) * BUFFER_LEN)) == NULL) {
            printf("unable to allocate memory\n");
            return -1;
        } 
    }
	
	/* Writing entry in to pointers */
	int count = readConfigFile(ipAddress, portNumber);
	
	
	/* Variable declaration */
	int sd;
	int rc; // return code for recvfrom
	struct sockaddr_in server_address;
	char buffer[100]; 
	
	/* Creating a socket */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		close(sd);
		exit(1);
	}
		
	/* Sending messages */
	memset(buffer, '\0', sizeof(buffer));
	while (1) {
		printf("Please input the message: (type \"STOP\" to end)\n");
		
		// Credit: Dr. Dave Ogle
		fgets(buffer, sizeof(buffer),stdin);
		buffer[strlen(buffer) - 1] = '\0'; //Change last char from \n to \0
		
		if (!strcmp(buffer, "STOP")) break;
		
		for (i = 0; i < (count / 2); i++){
			/* Setting IP address and port number */
			server_address.sin_family = AF_INET;
			server_address.sin_port = htons(*portNumber[i]);
			server_address.sin_addr.s_addr = inet_addr(ipAddress[i]);
			
			/* Sending message */
			rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
			if (rc < 0) {
				perror("sendto");
				close(sd);
				return -1;
			}
			printf("%d bytes sent to IP: %s, port %d.\n", rc, ipAddress[i], *portNumber[i]);
		}
	}

	close(sd);
	return 0;
}

/*
	This function will read the entry from config.txt and validate the format.
*/
int readConfigFile(char* ipAddress[], int* portNumber[]) {
	/* Reading from config.txt */
	FILE *fp;
	char* end;
	char dataBuffer[BUFFER_LEN];
	int count = 0;
	
	memset(dataBuffer, '\0', sizeof(dataBuffer));
	fp = fopen("./config.txt", "r");   
	while (fscanf(fp, "%s", dataBuffer) != EOF) {
		int setNum = (int) (count / 2);
		// If odd number then it should be IP address, else should be port number
		if (count % 2 == 0){
			if (!isValidIpAddr(dataBuffer)) {
				printf("Error: invalid <ipaddr>\nCheck config.txt, Usage is: <ipaddr> <portNumber>\n");
				exit(1);
			}
			strcpy(ipAddress[setNum], dataBuffer);			
		} else {
			*portNumber[setNum] = strtol(dataBuffer, &end, 10);
			if (end == dataBuffer) {
				printf("Error: invalid <portNumber>\nCheck config.txt, Usage is: <ipaddr> <portNumber>\n");
				exit(1);
			}
		}
		count++;
	}
	
	fclose(fp);
	return count;
}

/*
	This function will check if a string is valid IP address.
*/
int isValidIpAddr(char *ipaddr){
	struct sockaddr_in sockaddr;
	int result = inet_pton(AF_INET, ipaddr, &(sockaddr.sin_addr));
	return result == 1;
}


