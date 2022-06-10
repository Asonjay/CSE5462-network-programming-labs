/*
Title: 
	CSE5462-Lab2: client2.c
Author: 
	Jason Xu (xu.2460)
Description:
	This program creates a STREAM socket client. 
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
	sd = socket(AF_INET, SOCK_STREAM, 0);
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
		
	/* Connecting */
	rc = connect(sd, (struct sockaddr *)&server_address, sizeof(server_address));
	if(rc < 0){
		perror("connect");
		close(sd);
		exit(1);
	}
	printf("Connected to the server.\n");
	
	/* Sending messages */
	memset(buffer, '\0', sizeof(buffer));
	while(1){
		printf("Please input the message: (type \"STOP\" to end)\n");
		// Credit: Dr. Dave Ogle
		fgets(buffer, sizeof(buffer),stdin);
		buffer[strlen(buffer) - 1] = '\0'; //Change last char from \n to \0
		if(!strcmp(buffer, "STOP"))
		{
			close(sd);
			break;
		}
		// send instead of sentto
		rc = send(sd, buffer, strlen(buffer), 0);
		if(rc < 0){
			perror("send");
			close(sd);
			return -1;
		}
		if(rc != 0)
			printf("%d bytes sent.\n", rc);
	}
	
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


