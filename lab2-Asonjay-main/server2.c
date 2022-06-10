/*
Title: 
	CSE5462-Lab2: server2.c
Author: 
	Jason Xu (xu.2460)
Description:
	This program creates a STREAM socket server. 
Credit:
	Dr. Dave Ogle 
*/

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	/* Check input */
	if(argc != 2){
		printf("Usage is: server <portNumber>\n");
		exit(1);
	}
	
	/* Variable declaration */
	int sd; 
	int rc; // return code for recvfrom
	struct sockaddr_in server_address;
	struct sockaddr_in from_address; // Addr for recvfrom
	int portNumber;
	char buffer[100];
	socklen_t fromLength = sizeof(struct sockaddr);
	
	int new_sd;

	/* Creating a socket */	
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0){
		perror("socket");
		close(sd);
		exit(1);
	}
	
	/* Setting address of the server */
	portNumber = atoi(argv[1]);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portNumber);
	server_address.sin_addr.s_addr = INADDR_ANY;
	
	/* Bind: Assign address to the unbound socket */ 
	if(bind(sd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){
		perror("bind");
		close(sd);
		exit(1);
	}
	
	/* Listening */
	rc = listen(sd, 5);
	if(rc < 0){
		perror("listen");
		close(sd);
		exit(1);
	}
	
	/* Connection & receiving */
	while(1){
		printf("Awaiting for connection...\n");
		// new_sd 
		new_sd = accept(sd, (struct sockaddr *)&from_address, &fromLength);
		if(new_sd < 0){
			perror("accept");
			close(sd);
			close(new_sd);
			exit(1);
		}
		printf("Client Connected.\n");
		
		while(1){
			printf("Awaiting for message...\n");
			memset(buffer, '\0' ,sizeof(buffer));
			// recv instead of recvfrom
			rc = recv(new_sd, buffer, sizeof(buffer), 0);
			if(rc <= 0){
				printf("Client disconnected.\n");
				close(new_sd);
				break;
			}
			printf("Received a %d bytes message:\n%s\n", rc, buffer);
		}
	}
	
	close(sd);	
	return 0;
}
