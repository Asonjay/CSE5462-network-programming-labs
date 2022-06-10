/*
Title: 
	CSE5462-Lab1: client.c
Author: 
	Jason Xu (xu.2460)
Description:
	This program creates a DGRAM socket client. 
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

int isValidIpAddr(char *ipaddr);

int main(int argc, char *argv[])
{
	/* Check input */
	if(argc < 3){
		printf("Usage is: client <ipaddr> <portNumber>\n");
		exit(1);
	}
	// Validate whether second argument is a valid IP address
	if(!isValidIpAddr(argv[1])){
		printf("Error: invalid <ipaddr>\n");
		exit(1);
	}
	
	/* Variable declaration */
	int sd;
	int rc; // return code for recvfrom
	struct sockaddr_in server_address;
	int portNumber;
	char serverIP[29];
	char buffer[100]; 
	
	/* Creating a socket */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0){
		perror("socket");
		close(sd);
		exit(1);
	}
	
	/* Setting IP address and port number */
	portNumber = strtol(argv[2], NULL, 10);
	strcpy(serverIP, argv[1]);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portNumber);
	server_address.sin_addr.s_addr = inet_addr(serverIP);
		
	/* Sending messages */
	memset(buffer, '\0', sizeof(buffer));
	while(1){
		printf("Please input the message: (type \"STOP\" to end)\n");
		
		// Credit: Dr. Dave Ogle
		fgets(buffer, sizeof(buffer),stdin);
		buffer[strlen(buffer) - 1] = '\0'; //Change last char from \n to \0
		
		if(!strcmp(buffer, "STOP")) break;
		rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
		if(rc < 0){
			perror("sendto");
			close(sd);
			return -1;
		}
		printf("%d bytes sent.\n", rc);
	}

	close(sd);
	return 0;
}

/*
	This function will check if a string is valid IP address.
*/
int isValidIpAddr(char *ipaddr){
	struct sockaddr_in sockaddr;
	int result = inet_pton(AF_INET, ipaddr, &(sockaddr.sin_addr));
	return result == 1;
}


