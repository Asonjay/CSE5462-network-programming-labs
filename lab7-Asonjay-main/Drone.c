/************************************************************\
 * Title:                                                   *
 *	 CSE5462-Lab7: Drone.c                                  *
 * Author:                                                  *
 *	 Jason Xu (xu.2460)                                     *
 * Description:                                             *
 *	 This program integrates a DGRAM client and server with *
 *   protocol version 3.                                    *
\************************************************************/

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>

#include "InputUtility.h"
#include "GridUtility.h"


#define MAX_ENTRY 30
#define BUFFER_LEN 29
#define MSG_LEN 150
#define RT_LEN 25
#define STDIN 0
#define VERSION 3 
#define DISTANCE 2
#define HOPCOUNT 4 // 4 hops


void print_block(char* title);
void send_msg(char* buffer, int sd, struct sockaddr_in server_address);

/**main starts**/
int main (int argc, char *argv[])
{	
	/*1* Check validility of the input */
	if(argc != 2) print_input_error_msg("Error: Usage is: Drone <portNumber>\n");
	
	
	/*2* Read entries from config.txt */
	int i;
	char* ipAddress[MAX_ENTRY];
	int* portNumber[MAX_ENTRY];
	int* location[MAX_ENTRY];
	int* msgID[MAX_ENTRY];
	int numHosts = read_config_file(ipAddress, portNumber, location, msgID, MAX_ENTRY, BUFFER_LEN);
	print_block("READING config.txt");
	for (i = 0; i < numHosts; i++)
		printf("Location %d @ <%s:%d>\n", *location[i], ipAddress[i], *portNumber[i]);
	printf("\n");
		
	/*3* Read <portnumber> from command line and check input validality */
	int loc = -1;
	int locIdx = -1;
	char* currIpAddr;
	int currPortNum;
	int readPort = strtol(argv[1], NULL, 10); //
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
	if (loc == -1) print_input_error_msg("Error: No match of <portNumber> found\n");
		
			
	/*4* Read and init grid from input */ 
	int row = 10; // hard code to 10 3
	int col = 3;  // hard code to 10 3
	print_block("GRID PARAMETERS");
	printf("(Usage is: <row> <column>)\n:");
	//if (scanf("%d %d", &row, &col) <= 0)
	//	printInputErrorMsg("Error: Usage is <row> <Column>\n");
	printf("\n\n");
	print_block("PRINTING GRID");
	read_grid_data(row, col, loc, DISTANCE);
	printf("Curret Location: %d ", loc);
	printf("@ <%s:%d>\n", currIpAddr, currPortNum);	
	printf("\n");
	

	
	
	/*5* Creating a socket */
	int sd;
	int rc; // return code for recvfrom
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
	
	
	/*6* Bind: Assign address to the unbound socket */ 
	if(bind(sd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("bind");
		close(sd);
		exit(1);
	}
	
	
	/*7* Select: STDIN or SD mode */
	char buffer[MSG_LEN + 6]; // version(1) + spc(1) + loc(3) + spc(1)
	char msgRaw[MSG_LEN], msg[MSG_LEN];
	int msgLen;
	// Msg segment variables
	int recvVer, recvLoc, recvMsgID;
	int fromPortNum = -1, toPortNum = -1;
	int toLoc = -1;
	int toIdx = -1;
	int hopCount;
	// SD set
	fd_set sd_set; //socket descriptor set
	int maxSD; //max socket descriptor
	// Protocol Ver 3
	char msgType[3]; // "MSG" or "ACK"
	char route[RT_LEN], newRoute[RT_LEN];
	print_block("ENTERING MSG");
	printf("(Usage is: <portNum> <msg>)\n");
	while (1) 
	{
		fflush(stdout);

		memset(buffer, '\0', sizeof(buffer));
		memset(msgRaw, '\0', sizeof(msgRaw));
		memset(msg, '\0', sizeof(msg));
		memset(msgType, '\0', sizeof(msgType));
		memset(route, '\0', sizeof(route));
		
		FD_ZERO(&sd_set);
		FD_SET(sd, &sd_set);
		FD_SET(STDIN, &sd_set);
		maxSD = (STDIN > sd) ? STDIN : sd;
		rc = select(maxSD+1, &sd_set, NULL, NULL, NULL);
		
		/* Client mode if STDIN detected */
		if (FD_ISSET(STDIN, &sd_set)) 
		{
			fgets(msgRaw, sizeof(msgRaw), stdin);
			msgRaw[strlen(msgRaw) - 1] = '\0'; //Change last char from \n to \0

			if (!strcmp(msgRaw, "STOP")) break;
			
			sscanf(msgRaw, "%d %[^\n]s", &toPortNum, msg);
			
			sprintf(msgType, "%s", "MSG");
			msgLen = strlen(msg);
			if (strlen(msg) > 0)
			{	
				toLoc = -1;		
				for (i = 0; i < numHosts; i++) 
				{
					if (toPortNum == *portNumber[i]) 
					{
						toLoc = *location[i];
						toIdx = i;                                    
					}                            
				}
				if (toLoc == -1) print_input_error_msg("Error: No match of <portNumber> found\n");
				
				sprintf(route, "%d", currPortNum);
				// buffer = "<ver> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>"
				sprintf(buffer, "%d %d %d %d %d %s %d %s %s", VERSION, loc, toPortNum, currPortNum, HOPCOUNT, msgType, (*msgID[toIdx])++, route, msg); 
				
				//printf("\"%s\"\n", buffer);
				// Send directly if dest in range
				if (check_distance(loc, toLoc, row, col, DISTANCE)) 
				{
					server_address.sin_family = AF_INET;
					server_address.sin_port = htons(*portNumber[toIdx]);
					server_address.sin_addr.s_addr = inet_addr(ipAddress[toIdx]);
					send_msg(buffer, sd, server_address);
					printf("  [SENT]->\"DEST_IN_RANGE\"\n");
					printf("    %d bytes sent to location %d @ <%s:%d>\n", msgLen, *location[toIdx], ipAddress[toIdx], *portNumber[toIdx]);
				} 
				else 
				{
					printf("  [SENT]->\"BROADCAST\"\n");
					for (i = 0; i < numHosts; i++)
					{
						if (i == locIdx) continue;
						server_address.sin_family = AF_INET;
						server_address.sin_port = htons(*portNumber[i]);
						server_address.sin_addr.s_addr = inet_addr(ipAddress[i]);
						send_msg(buffer, sd, server_address);
						printf("    %d bytes sent to location %d @ <%s:%d>\n", msgLen, *location[i], ipAddress[i], *portNumber[i]);
					}
				}
				printf("\n");
			}
		}
		
		/* Server mode if not STDIN */
		if (FD_ISSET(sd, &sd_set)) 
		{
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

			//printf("\"%s\"\n", buffer);
			/* Resend info if in range */
			if(check_distance(loc, recvLoc, row, col, DISTANCE) && hopCount > 0) 
			{
				printf(":%s\n", msg);
				printf("  [RECEIVED]<-\"IN_RANGE\"\n");
				printf("    %d bytes from location %d <ver=%d> <type=%s> <hop_count=%d>\n\n", msgLen, recvLoc, recvVer, msgType, hopCount);

				int ackFlag = 1; // 1: normal MSG, 0: receive ACK back, 2: sending ACK
				
				hopCount--;
				// Receive msg and send ACK
				if (toPortNum == currPortNum)
				{	
					if (!strcmp(msgType, "ACK")) 
					{
						// if ACK, then, then receive ACK back stop resending
						ackFlag = 0;
					}	
					else 
					{
						ackFlag = 2;
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
					sprintf(newRoute, "%s,%d", route, currPortNum);
					sprintf(buffer, "%d %d %d %d %d %s %d %s %s", VERSION, loc, toPortNum, fromPortNum, hopCount, msgType, recvMsgID, newRoute, msg);
				}
				
				if (ackFlag > 0) 
				{
					// swap to address
					if (ackFlag == 2) 
					{
						toPortNum = fromPortNum;
					}
					
					for (i = 0; i < numHosts; i++) 
					{
						if (toPortNum == *portNumber[i]) 
						{
							toLoc = *location[i];
							toIdx = i;
						}
					}
					
					//printf("\"%s\"\n", buffer);
					// Send directly if dest in range
					if (check_distance(loc, toLoc, row, col, DISTANCE) && hopCount > 0) 
					{
						server_address.sin_family = AF_INET;
						server_address.sin_port = htons(*portNumber[toIdx]);
						server_address.sin_addr.s_addr = inet_addr(ipAddress[toIdx]);
						send_msg(buffer, sd, server_address);
						if (!strcmp(msgType, "ACK")) 
							printf("  [ACK]->\"DEST_IN_RANGE\"\n");
						else
							printf("  [SENT]->\"DEST_IN_RANGE\"\n");
						printf("    %d bytes sent to location %d @ <%s:%d>\n", msgLen, *location[toIdx], ipAddress[toIdx], *portNumber[toIdx]);
					} 
					else 
					{
						if (!strcmp(msgType, "ACK")) 
							printf("  [ACK]->\"BROADCAST\"\n");
						else
							printf("  [SENT]->\"BROADCAST\"\n");
						for (i = 0; i < numHosts; i++)
						{
							if (i == locIdx) continue;
							server_address.sin_family = AF_INET;
							server_address.sin_port = htons(*portNumber[i]);
							server_address.sin_addr.s_addr = inet_addr(ipAddress[i]);
							send_msg(buffer, sd, server_address);	
							printf("    %d bytes sent to location %d @ <%s:%d>\n", msgLen, *location[i], ipAddress[i], *portNumber[i]);
						}
					}
					printf("\n");
				}		
			}
		}
	}
	close(sd);
	
	
	/*8* Exiting */
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

void send_msg(char* buffer, int sd, struct sockaddr_in server_address)
{
	int rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	if (rc < 0) 
	{
		close(sd);
		perror("sendto");
		exit(-1);
	}	
}


