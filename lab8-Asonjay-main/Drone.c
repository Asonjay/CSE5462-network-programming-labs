/************************************************************\
 * Title:                                                   *
 *	 CSE5462-Lab8: Drone.c                                  *
 * Author:                                                  *
 *	 Jason Xu (xu.2460)                                     *
 * Description:                                             *
 *	 This program integrates a DGRAM client and server with *
 *   protocol version 4.                                    *
\************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>

#include "InputUtility.h"
#include "GridUtility.h"

#define MAX_ENTRY 30
#define BUFFER_LEN 29
#define MSG_LEN 200
#define RT_LEN 25
#define STDIN 0
#define MSG 0
#define ACK 1
#define MOV 2
#define VERSION 4
#define DISTANCE 2
#define HOPCOUNT 4 // 4 hops


void print_block(char* title);
void send_msg(char* buffer, int sd, int msgLen, struct sockaddr_in server_address, int loc, int port, char* ipAddr);
int get_msg_type(char* msgType);

/**main starts**/
int main (int argc, char *argv[])
{	
	/***********************************
	 *1* Check validility of the input *
	 ***********************************/
	if(argc != 2) print_input_error_msg("[Error] Usage is: Drone <portNumber>\n");
	int readPort = strtol(argv[1], NULL, 10); 

	/**********************************
	 *2* Read entries from config.txt *
	 **********************************/
	int i;
	char* ipAddress[MAX_ENTRY];
	int* portNumber[MAX_ENTRY];
	int* location[MAX_ENTRY];
	int* msgID[MAX_ENTRY];
	int row, col;
	char* currIpAddr;
	int currPortNum;
	int loc = -1, locIdx = -1;
	/* Reading Config file*/
	int numHosts = read_config_file(ipAddress, portNumber, location, msgID, &row, &col, MAX_ENTRY, BUFFER_LEN);
	print_block("READING config.txt");
	for (i = 0; i < numHosts; i++) printf("Location %d @ <%s:%d>\n", *location[i], ipAddress[i], *portNumber[i]);
	printf("\n");
	/* Printing Grid */
	print_block("PRINTING GRID");
	read_grid_data(row, col, loc, DISTANCE);
	/* Getting current location */
	for (i = 0; i < numHosts; i++) 
	{
		if (readPort == *portNumber[i]) 
		{
			loc = *location[i];
			locIdx = i;
			currIpAddr = ipAddress[i];
			currPortNum = *portNumber[i];
		}
	}
	if (loc == -1) print_input_error_msg("[Error] No match of <portNumber> found\n");
	printf("Curret Location: %d ", loc);
	printf("@ <%s:%d>\n", currIpAddr, currPortNum);	
	printf("\n");
		
	/***********************************
	 *3* Creating a socket and binding *
	 ***********************************/
	int sd, rc;
	struct sockaddr_in server_address;
	struct sockaddr_in from_address;
	socklen_t fromLength = sizeof(struct sockaddr);
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) 
	{
		perror("socket");
		close(sd);
		exit(-1);
	}
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(currPortNum);
	server_address.sin_addr.s_addr = INADDR_ANY;
	if(bind(sd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("bind");
		close(sd);
		exit(1);
	}
	
	/******************************
	 *4* Select: STDIN or SD mode *
	 ******************************/
	char buffer[MSG_LEN], msgRaw[MSG_LEN], msg[MSG_LEN], portBuffer[MSG_LEN];
	int msgLen;
	// Msg segment variables
	int recvVer, recvLoc, recvMsgID;
	int fromPortNum = -1, toPortNum = -1;
	int toLoc = -1, toIdx = -1;
	int hopCount;
	// SD set
	fd_set sd_set; //socket descriptor set
	int maxSD; //max socket descriptor
	char msgType[3], typeCheck[5]; // "MSG" or "ACK" or "MOV"
	char route[RT_LEN], newRoute[RT_LEN];
	// Move
	int moveDest;
	/* Loop for selection */
	print_block("Usage is: <portNum> <msg>");
	while (1) 
	{
		fflush(stdout);
		/* Memset all buffer */
		memset(buffer, '\0', sizeof(buffer));
		memset(msgRaw, '\0', sizeof(msgRaw));
		memset(msg, '\0', sizeof(msg));
		memset(msgType, '\0', sizeof(msgType));
		memset(route, '\0', sizeof(route));
		memset(portBuffer, '\0', sizeof(portBuffer));
		/* Init FD for selection */
		FD_ZERO(&sd_set);
		FD_SET(sd, &sd_set);
		FD_SET(STDIN, &sd_set);
		maxSD = (STDIN > sd) ? STDIN : sd;
		rc = select(maxSD+1, &sd_set, NULL, NULL, NULL);
		/*********************************
		 * Client mode if STDIN detected *
		 *********************************/
		if (FD_ISSET(STDIN, &sd_set)) 
		{
			/* Read user input */
			fgets(msgRaw, sizeof(msgRaw), stdin);
			msgRaw[strlen(msgRaw) - 1] = '\0'; //Change last char from \n to \0
			/* Exit if STOP is read */
			if (!strcmp(msgRaw, "STOP")) break;
			/* Parse msg part */
			if (sscanf(msgRaw, "%d %[^\n]s", &toPortNum, msg) != 2) print_input_error_msg("[Error] Usage is: <portNum> <msg>\n");
			/* Lookup to-location */
			toLoc = -1;		
			for (i = 0; i < numHosts; i++) 
			{
				if (toPortNum == *portNumber[i]) 
				{
					toLoc = *location[i];
					toIdx = i;                                    
				}                            
			}
			if (toLoc == -1) print_input_error_msg("[Error] No match of <portNumber> found\n");
			/* Check if it is MOV command */
			snprintf(typeCheck, sizeof(typeCheck), "%.4s", msg);
			if (!strcmp(typeCheck, "MOV ")) 
			{
				sprintf(msgType, "%s", "MOV");
				if (sscanf(msg, "%s %d", msgType, &moveDest) != 2) print_input_error_msg("[Error] Usage is: <MOV> <location>\n");
				sprintf(msg, "%d", moveDest);
			}
			else sprintf(msgType, "%s", "MSG");
			/* Send msg */
			msgLen = strlen(msg);
			if (strlen(msg) > 0)
			{	
				sprintf(route, "%d", currPortNum);
				// buffer = "<ver> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>"
				sprintf(buffer, "%d %d %d %d %d %s %d %s %s", VERSION, loc, toPortNum, currPortNum, HOPCOUNT, msgType, *msgID[toIdx], route, msg); 
				if (!strcmp(msgType, "MSG")) (*msgID[toIdx])++;
				switch (get_msg_type(msgType))
				{
					case MSG:
					{
						/* Send directly if destition in range */
						if (check_distance(loc, toLoc, row, col, DISTANCE)) 
						{
							printf("  [SENT]->\"DEST_IN_RANGE\"\n");
							send_msg(buffer, msgLen, sd, server_address, *location[toIdx], *portNumber[toIdx], ipAddress[toIdx]);
						} 
						/* Broadcast otherwise*/
						else 
						{
							printf("  [SENT]->\"BROADCAST\"\n");
							for (i = 0; i < numHosts; i++)
							{
								if (i == locIdx) continue;
								send_msg(buffer, msgLen, sd, server_address, *location[i], *portNumber[i], ipAddress[i]);
							}
						}
						break;
					}
					case MOV:
					{
						printf("  [MOV]->\"PORT %d TO LOC %d\"\n", toPortNum, moveDest);
						send_msg(buffer, msgLen, sd, server_address, *location[toIdx], *portNumber[toIdx], ipAddress[toIdx]);
						*location[toIdx] = moveDest;
						break;
					}
					default:
						break;
				}
				printf("\n");
			}
		}
		/****************************
		 * Server mode if not STDIN *
		 ****************************/
		if (FD_ISSET(sd, &sd_set)) 
		{
			print_block("Selection popped");
			/* Receiving Stuff */
			rc = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_address, &fromLength);
			if(rc < 0) 
			{
				close(sd);
				perror("recvfrom");
				return -1;
			}
			// buffer = "<ver> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>"
			sscanf(buffer, "%d %d %d %d %d %s %d %s %[^\n]s", &recvVer, &recvLoc, &toPortNum, &fromPortNum, &hopCount, msgType, &recvMsgID, route, msg);
			msgLen = strlen(msg);

			/* Check if it is MOV */
			if (!strcmp(msgType, "MOV"))
			{
				if (toPortNum == currPortNum)
				{
					loc = atoi(msg);
					*location[locIdx] = atoi(msg);
					printf(":%s\n", msg);
					printf("  [RECEIVED]<-\"MOV\"\n");
					printf("    %d bytes from <LOC:%d> <TYPE:%s>\n", msgLen, recvLoc, msgType);
					printf("    Port %d move to location %d\n\n", currPortNum, loc);
				}
			}
			/* Resend info if in range */
			else if (check_distance(loc, recvLoc, row, col, DISTANCE) && hopCount > 0) 
			{
				printf(":%s\n", msg);
				printf("  [RECEIVED]<-\"IN_RANGE\"\n");
				printf("    %d bytes from <LOC:%d> <TYPE:%s>\n\n", msgLen, recvLoc, msgType);

				int flag = 1; // -1: route include curr port num, 1: normal MSG, 0: receive ACK back, 2: sending ACK
				
				hopCount--;
				// Receive msg and send ACK
				if (toPortNum == currPortNum)
				{	
					if (!strcmp(msgType, "ACK")) 
						flag = 0;
					else 
					{
						flag = 2;
						hopCount = HOPCOUNT;
						// else MSG, modify MSG resend back
						memset(msgType, '\0', sizeof(msgType));
						sprintf(msgType, "%s", "ACK");
						memset(route, '\0', sizeof(route));
						sprintf(route, "%d", currPortNum);
						memset(msg, '\0', sizeof(msg));
						sprintf(msg, "%s from %d", "(ACK)", toPortNum);
						memset(buffer, '\0', sizeof(buffer));
						sprintf(buffer, "%d %d %d %d %d %s %d %s %s", VERSION, loc, fromPortNum, toPortNum, HOPCOUNT, msgType, recvMsgID, route, msg);
						msgLen = strlen(msg);
					}
				}
				else
				{	
					// Fowarding msg
					sprintf(portBuffer, "%d", currPortNum);
					if (strstr(route, portBuffer)) flag = -1;
					else
					{
						sprintf(newRoute, "%s,%d", route, currPortNum);
						sprintf(buffer, "%d %d %d %d %d %s %d %s %s", VERSION, loc, toPortNum, fromPortNum, hopCount, msgType, recvMsgID, newRoute, msg);
					}
				}
				
				if (flag > 0) 
				{
					// swap to address
					if (flag == 2) toPortNum = fromPortNum;
					for (i = 0; i < numHosts; i++) 
					{
						if (toPortNum == *portNumber[i]) 
						{
							toLoc = *location[i];
							toIdx = i;
						}
					}
					// Send directly if dest in range
					if (check_distance(loc, toLoc, row, col, DISTANCE) && hopCount > 0) 
					{	
						//printf("%s\n", buffer);
						sprintf(portBuffer, "%d", toPortNum);
						if (!strstr(route, portBuffer))
						{
							if (!strcmp(msgType, "ACK")) 
								printf("  [ACK]->\"DEST_IN_RANGE\"\n");
							else
								printf("  [SENT]->\"DEST_IN_RANGE\"\n");
							send_msg(buffer, msgLen, sd, server_address, *location[toIdx], *portNumber[toIdx], ipAddress[toIdx]);
						}
					} 
					else 
					{
						//printf("%s\n", buffer);
						if (!strcmp(msgType, "ACK")) 
							printf("  [ACK]->\"BROADCAST\"\n");
						else
							printf("  [SENT]->\"BROADCAST\"\n");
						for (i = 0; i < numHosts; i++)
						{
							if (i == locIdx) continue;
							sprintf(portBuffer, "%d", *portNumber[i]);
							if (strstr(route, portBuffer)) continue;
							send_msg(buffer, msgLen, sd, server_address, *location[i], *portNumber[i], ipAddress[i]);
						}
					}
					printf("\n");
				}		
			}
		}
		print_block("Usage is: <portNum> <msg>");
	}
	close(sd);
	
	/*************
	 *8* Exiting *
	 *************/
	printf("\n[Exiting]..\n");
	return 0;
}
/**main ends**/


void print_block(char* title)
{
	int len = strlen(title);
	int i = 0;
	for(i = 0; i < len + 4; i++)
		printf("#");
	printf("\n# %s #\n", title);
	for(i = 0; i < len + 4; i++)
		printf("#");
	printf("\n");
}

void send_msg(char* buffer, int msgLen, int sd, struct sockaddr_in server_address, int loc, int port, char* ipAddr)
{
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = inet_addr(ipAddr);
	int rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	if (rc < 0) 
	{
		close(sd);
		perror("sendto");
		exit(-1);
	}	
	printf("    %d bytes to <LOC:%d> @ <%s:%d>\n", msgLen, loc, ipAddr, port);
}

int get_msg_type(char* msgType)
{
	int rc;
	if (!strcmp(msgType, "MSG")) rc = MSG;
	if (!strcmp(msgType, "ACK")) rc = ACK;
	if (!strcmp(msgType, "MOV")) rc = MOV;
	return rc;
}

