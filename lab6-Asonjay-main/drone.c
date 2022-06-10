/******************************************************************
*	Title: 
*		CSE5462-Lab6: drone.c
*	Author: 
*		Jason Xu (xu.2460)
*	Description:
*		This program integrates a DGRAM client and server together
*	Credit:
*		Dr. Dave Ogle 
******************************************************************/

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
#define MSG_LEN 100
#define STDIN 0
#define VERSION 2
#define DISTANCE 2
#define HOPCOUNT 3

int readConfigFile(char* ipAddress[], int* portNumber[], int* location[]);
int isValidIpAddr (char* ipaddr);
void printInputErrorMsg(char* msg);
void readGridData(int row, int col, int currLoc);
void getCords(int grid_row, int grid_col, int* x, int* y, int loc);
int checkDistance(int currLoc, int tarLoc, int row, int col, int dist);


/**main starts**/
int main (int argc, char *argv[])
{	
	/* Check validility of the input */
	if(argc != 2) printInputErrorMsg("Error: Usage is: client <portNumber>\n");
	
	
	/* Read entries from config.txt */
	int i;
	char* ipAddress[MAX_ENTRY];
	int* portNumber[MAX_ENTRY];
	int* location[MAX_ENTRY];
	int numHosts = readConfigFile(ipAddress, portNumber, location);
	printf("#### Reading from config.txt ####\n");
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
	printf("#### Current Location ####\n");
	printf("Current Location is %d:\n", loc);
	printf("\t@ <%s:%d>\n", currIpAddr, currPortNum);	
	printf("\n");	
				
	/* Read and init grid from input */ 
	int row = 0;
	int col = 0;
	printf("#### Enter grid parameters ####\n");
	printf("Usage is <row> <column>:  \n");
	if (scanf("%d %d", &row, &col) <= 0)
		printInputErrorMsg("Error: Usage is <row> <Column>\n");
	readGridData(row, col, loc);
	
	
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
	char buffer[MSG_LEN+6]; // version(1) + spc(1) + loc(3) + spc(1)
	char msg_raw[MSG_LEN];
	char msg[MSG_LEN];
	int msg_len;
	// Msg segment variables
	int recvVer;
	int recvLoc;
	int fromPortNum = -1;
	int toPortNum = -1;
	int toLoc = -1;
	int toIdx = -1;
	
	int hopCount;
	printf("#### Enter the msg to send ####\n");
	printf("Usage is: <portNum> <msg>\n");
	while (1) {
		fflush(stdout);
		//printf("\n################################\n");

		memset(buffer, '\0', sizeof(buffer));
		memset(msg_raw, '\0', sizeof(msg_raw));
		memset(msg, '\0', sizeof(msg));
		
		FD_ZERO(&sd_set);
		FD_SET(sd, &sd_set);
		FD_SET(STDIN, &sd_set);
		maxSD = (STDIN > sd) ? STDIN : sd;
		rc = select(maxSD+1, &sd_set, NULL, NULL, NULL);
		
		/*
		 * Client mode if STDIN detected
		 */
		if (FD_ISSET(STDIN, &sd_set)) {
			fgets(msg_raw, sizeof(msg_raw), stdin);
			
			msg_raw[strlen(msg_raw) - 1] = '\0'; //Change last char from \n to \0

			if (!strcmp(msg_raw, "STOP")) break;
			
			sscanf(msg_raw, "%d %[^\n]s", &toPortNum, msg);
			// buffer = "<ver> <loc> <to> <from> <hopcount> <msg>"
			msg_len = strlen(msg);
			if (strlen(msg) > 0){	
					
				toLoc = -1;		
				for (i = 0; i < numHosts; i++) {
					if (toPortNum == *portNumber[i]) {
						toLoc = *location[i];
						toIdx = i;
					}
				}
				if (toLoc == -1) printInputErrorMsg("Error: No match of <portNumber> found\n");
			
				sprintf(buffer, "%d %d %d %d %d %s", VERSION, loc, toPortNum, currPortNum, HOPCOUNT, msg); 
				
				printf("\n[Sending] ->\n");
				// Send directly if dest in range
				if (checkDistance(loc, toLoc, row, col, DISTANCE)) {
					server_address.sin_family = AF_INET;
					server_address.sin_port = htons(*portNumber[toIdx]);
					server_address.sin_addr.s_addr = inet_addr(ipAddress[toIdx]);
					
					rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
					if (rc < 0) {
						close(sd);
						perror("sendto");
						return -1;
					}	
					printf("%d bytes sent to location %d\n\t@ <%s:%d>\n", msg_len, *location[toIdx], ipAddress[toIdx], *portNumber[toIdx]);
				} else {
					for (i = 0; i < numHosts; i++){
						if (i == locIdx) continue;
						
						/* Sending stuff */
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
					printf("\n");
				}
			}
		}
		
		/*
		 * Server mode if not STDIN 
		 */
		if (FD_ISSET(sd, &sd_set)) {
		
			/* Receiving Stuff */
			rc = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_address, &fromLength);
			if(rc < 0) {
				close(sd);
				perror("recvfrom");
				return -1;
			}
			// buffer = "<ver> <loc> <to> <from> <hopcount> <msg>"
			sscanf(buffer, "%d %d %d %d %d %[^\n]s", &recvVer, &recvLoc, &toPortNum, &fromPortNum, &hopCount, msg);
			msg_len = strlen(msg);
			// Resend info if in range
			if(checkDistance(loc, recvLoc, row, col, DISTANCE)) {
				printf("\n#### \"IN_RANGE\" from location %d ####\n", recvLoc);
				printf("[Receiving] <- %d bytes <ver.%d>:\n%s\n<hop count> = %d\n\n", msg_len, recvVer, msg, hopCount);
				

				if (hopCount > 0) {
					hopCount--;
					
					if (toPortNum == currPortNum) hopCount = 0;
					// buffer = "<ver> <loc> <to> <from> <hopcount> <msg>"
					if (strlen(msg) > 0){
						for (i = 0; i < numHosts; i++) {
							if (toPortNum == *portNumber[i]) {
								toLoc = *location[i];
								toIdx = i;
							}
						}
						sprintf(buffer, "%d %d %d %d %d %s", VERSION, loc, toPortNum, fromPortNum, hopCount, msg);
						
						printf("[Sending] ->\n");
						// Send directly if dest in range
						if (checkDistance(loc, toLoc, row, col, DISTANCE)) {
							server_address.sin_family = AF_INET;
							server_address.sin_port = htons(*portNumber[toIdx]);
							server_address.sin_addr.s_addr = inet_addr(ipAddress[toIdx]);
							
							rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
							if (rc < 0) {
								close(sd);
								perror("sendto");
								return -1;
							}	
							printf("%d bytes sent to location %d\n\t@ <%s:%d>\n", msg_len, *location[toIdx], ipAddress[toIdx], *portNumber[toIdx]);
						} else {
							for (i = 0; i < numHosts; i++){
								if (i == locIdx) continue;
								
								/* Sending stuff */
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
							printf("\n");
						}
					}
				/*
				// Send directly if dest in range
				if check(loc, toLoc, row, col, DISTANCE) {
					server_address.sin_family = AF_INET;
					server_address.sin_port = htons(*portNumber[toIdx]);
					server_address.sin_addr.s_addr = inet_addr(ipAddress[toIdx]);
					
					rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
					if (rc < 0) {
						close(sd);
						perror("sendto");
						return -1;
					}	
					printf("%d bytes sent to location %d\n\t@ <%s:%d>\n", msg_len, *location[i], ipAddress[i], *portNumber[i]);
				}*/
			
				}
			}
		}
	}
	close(sd);
	
	printf("\n[Exiting]..\n");
	return 0;
}
/**main ends**/


/*****************************************************	
*	This function will read the entry from config.txt 
*	and validate the format.
*	:return: 
*		number of entries
*****************************************************/
int readConfigFile(char* ipAddress[], int* portNumber[], int* location[]) {	
	// Writing entry in to pointers
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
	// Reading from config.txt
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

/***************************************************************
*	This function will check if a string is valid IPv4 address.
*	:return:
*		(<ipaddr> is valid)
***************************************************************/
int isValidIpAddr(char *ipaddr){
	struct sockaddr_in sockaddr;
	int result = inet_pton(AF_INET, ipaddr, &(sockaddr.sin_addr));
	return result == 1;
}

/*********************************************************************
*	Print error msg from invalid usage of command line and config.txt
*	Error code: 1
*********************************************************************/
void printInputErrorMsg(char* msg){
	printf(msg);
	exit(1);
}

/********************************************
*	Read and setup the grid from user inputs
*	Notes: 3*3 with location 5
*			currRow = 5 / 3 = 1
*			currCol = 5 % 3 = 2 (actually 1)
*   :print example:
*	+---+---+---+
*	|001|002|003|
*   +---+---+---+
*	|004|***|006|
*  	+---+---+---+
*	|007|008|009|
*   +---+---+---+
********************************************/
void readGridData(int row, int col, int currLoc) {
	int i;
	int j;
	int currRow;
	int currCol;
	// Calculate curr row & col
	getCords(row, col, &currRow, &currCol, currLoc);
	
	printf("\n#### Print Grid ####\n");
	printf("(***): current location\n");
	printf("[Creating Grid]:\n");
	
	// Print the rest
	for (i = 0; i < row; i++) {
		// Grid Line
		printf("+");
		for (j = 0; j < col; j++) 
			printf("---+");
		printf("\n");
		// Number Line
		printf("|");
		for (j = 0; j < col; j++) {
			if(i == currRow && j == currCol){
				printf("***|");
				continue;
			}
			printf("%03d|", i*(col)+(j+1));
		}
		printf("\n");
	}
	// Print Last grid line
	printf("+");
	for (i = 0; i < col; i++) 
		printf("---+");
	printf("\n");
	printf("DISTANCE set to %d\n\n", DISTANCE);
}

/***************************************************************
*	Calculate row and col based on given grid size and location
*	Notes:
*		(7, 6) 25(4, 0) -> (25 / 6 = 4, 25 % 6 = 1)
*		(7, 6) 24(3, 5) -> (24 / 6 = 4 - 1, 6 - 1 = 5)
***************************************************************/
void getCords(int grid_row, int grid_col, int* x, int* y, int loc) {
	if (loc % grid_col == 0) {
		*x = loc / grid_col - 1;
		*y = grid_col - 1;
	} else {
		*x = (int) (loc / grid_col);
		*y = loc % grid_col - 1;
	}
}

/*******************************************************************
*	Check if tarLoc is within the distance of currLoc with Euc_Dist
* 	:return:
*		(tarLoc is within the distance of currLoc)
*******************************************************************/
int checkDistance(int currLoc, int tarLoc, int row, int col, int dist) {
	int curr_x;
	int curr_y;
	int tar_x;
	int tar_y;
	getCords(row, col, &curr_x, &curr_y, currLoc);
	getCords(row, col, &tar_x, &tar_y, tarLoc);
	return (int) sqrt((double)(pow(curr_x - tar_x, 2) + pow(curr_y - tar_y, 2))) <= dist;
}

