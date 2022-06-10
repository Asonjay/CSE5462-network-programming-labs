/************************************************************\
 * Title:                                                   *
 *	 CSE5462-Lab9: Drone.c                                  *
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
#include <time.h>
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
#define TIMEOUT 5
#define RESEND 5
#define MAX_STORE 30

typedef struct MsgStore {
    int resend;
    char msg[MSG_LEN];
	int toPortNum;
	char route[RT_LEN];
} MsgStore;

void print_block(char* title);
void send_msg(char* buffer, int sd, struct sockaddr_in server_address, int loc, int port, char* ipAddr);
int get_msg_type(char* msgType);
void msg_store_write(MsgStore msgStore[], int* store_idx[], int* store_len, char* buffer, int toPortNum, char* route);
void msg_store_send(MsgStore msgStore[], int* store_idx[], int* store_len, int sd, struct sockaddr_in server_address, int* location[], int* portNumber[], char* ipAddress[], int numHosts, int locIdx, int row, int col);
void msg_store_delete(MsgStore msgStore[], int* store_idx[], int* store_len, int i);


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
	/*
	for (i = 0; i < numHosts; i++) printf("Location %d @ <%s:%d>\n", *location[i], ipAddress[i], *portNumber[i]);
	printf("\n");
	print_block("PRINTING GRID");
	read_grid_data(row, col, loc, DISTANCE);
	*/
	/* Getting current location */
	for (i = 0; i < numHosts; i++) {
		if (readPort == *portNumber[i]) {
			loc = *location[i];
			locIdx = i;
			currIpAddr = ipAddress[i];
			currPortNum = *portNumber[i];
		}
	}
	if (loc == -1) print_input_error_msg("[Error] No match of <portNumber> found\n");
	printf("Curret Location: %d @ <%s:%d>\n", loc, currIpAddr, currPortNum);

	/***********************************
	 *3* Creating a socket and binding *
	 ***********************************/
	int sd, rc;
	struct sockaddr_in server_address;
	struct sockaddr_in from_address;
	socklen_t fromLength = sizeof(struct sockaddr);
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) { perror("socket"); close(sd); exit(-1); }
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(currPortNum);
	server_address.sin_addr.s_addr = INADDR_ANY;
	if(bind(sd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
		perror("bind");
		close(sd);
		exit(1);
	}

	/******************************
	 *4* Select: STDIN or SD mode *
	 ******************************/
	/* SD set */
	fd_set sd_set; //socket descriptor set
	int maxSD; //max socket descriptor

	/* MSG buffer */
	char buffer[MSG_LEN], msgRaw[MSG_LEN], msg[MSG_LEN], portBuffer[MSG_LEN];
	int msgLen;

	/* Msg segment variables */
	int recvVer, recvLoc, recvMsgID;
	int fromPortNum = -1, toPortNum = -1;
	int toLoc = -1, toIdx = -1;
	int hopCount;
	char msgType[3], typeCheck[5]; // "MSG" or "ACK" or "MOV"
	char route[RT_LEN], newRoute[RT_LEN];
	int moveDest;

	/* Msg store */
	int store_len = 0;
	int* store_idx[MAX_STORE];
	for (i = 0; i < MAX_STORE; i++)	{
		if ((store_idx[i] = malloc(sizeof(int) * MAX_STORE)) == NULL) 
			{ perror("malloc"); return -1; }
		*store_idx[i] = -1;
	}
	MsgStore msgStore[MAX_STORE];
	int t_v, t_loc, t_to, t_from, t_hop, t_id;
	char t_type[3], t_route[RT_LEN], t_msg[MSG_LEN];
	/* Time */
	time_t begin = time(NULL);
	/* Loop for selection */
	print_block("USAGE: <portNum> <msg>");
	while (1) {
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
		struct timeval timeout = {TIMEOUT, 0};
		rc = select(maxSD+1, &sd_set, NULL, NULL, &timeout);

		/*********************
		 * Cycling Msg Store *
		 *********************/
		if (time(NULL) - begin > TIMEOUT)
			if (store_len > 0) { 
				msg_store_send(msgStore, store_idx, &store_len, sd, server_address, location, portNumber, ipAddress, numHosts, locIdx, row, col);
				begin = time(NULL);
			}

		/*********************************
		 * Client mode if STDIN detected *
		 *********************************/
		if (FD_ISSET(STDIN, &sd_set)) {
			/* Read user input */
			fgets(msgRaw, sizeof(msgRaw), stdin);
			msgRaw[strlen(msgRaw) - 1] = '\0'; //Change last char from \n to \0

			/* Exit if STOP is read */
			if (!strcmp(msgRaw, "STOP")) break;

			/* Parse msg part */
			if (sscanf(msgRaw, "%d %[^\n]s", &toPortNum, msg) != 2) print_input_error_msg("[Error] Usage is: <portNum> <msg>\n");

			/* Lookup to-location */
			toLoc = -1;		
			for (i = 0; i < numHosts; i++) {
				if (toPortNum == *portNumber[i]) {
					toLoc = *location[i];
					toIdx = i;                                    
				}                            
			}
			if (toLoc == -1) print_input_error_msg("[Error] No match of <portNumber> found\n");

			/* Check if it is MOV command */
			snprintf(typeCheck, sizeof(typeCheck), "%.4s", msg);
			if (!strcmp(typeCheck, "MOV ")) {
				sprintf(msgType, "%s", "MOV");
				if (sscanf(msg, "%s %d", msgType, &moveDest) != 2) print_input_error_msg("[Error] Usage is: <MOV> <location>\n");
				sprintf(msg, "%d", moveDest);
			}
			else 
				sprintf(msgType, "%s", "MSG");

			/* Send msg */
			msgLen = strlen(msg);
			if (strlen(msg) > 0) {	
				sprintf(route, "%d", currPortNum);
				// buffer = "<ver> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>"
				sprintf(buffer, "%d %d %d %d %d %s %d %s %s", VERSION, loc, toPortNum, currPortNum, HOPCOUNT, msgType, *msgID[toIdx], route, msg); 
				
				if (!strcmp(msgType, "MSG")) (*msgID[toIdx])++;
				// MSG or MOV
				switch (get_msg_type(msgType)) {
					case MSG: {
						if (store_len < MAX_STORE)
							msg_store_write(msgStore, store_idx, &store_len, buffer, toPortNum, route);
						break;
					}
					case MOV: {
						printf("[MOV] Port %d to %d\n", *portNumber[toIdx], moveDest);
						for (i = 0; i < numHosts; i++) {
							if (i == locIdx) continue;
							send_msg(buffer, sd, server_address, loc, *portNumber[i], ipAddress[i]);
						}
						*location[toIdx] = moveDest;
						break;
					}
					default:
						break;
				}
			}
		}

		/****************************
		 * Server mode if not STDIN *
		 ****************************/
		if (FD_ISSET(sd, &sd_set)) {
			/* Receiving Stuff */
			rc = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_address, &fromLength);
			if (rc < 0) { close(sd); perror("recvfrom"); return -1; }
			// printf("RECV %s\n", buffer);
			// buffer = "<ver> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>"
			sscanf(buffer, "%d %d %d %d %d %s %d %s %[^\n]s", &recvVer, &recvLoc, &toPortNum, &fromPortNum, &hopCount, msgType, &recvMsgID, route, msg);
			msgLen = strlen(msg);
			if (msgLen > 0) {
				/* Received MOV */
				if (!strcmp(msgType, "MOV")) {
					if (toPortNum == currPortNum) {
						loc = atoi(msg);
						*location[locIdx] = atoi(msg);
						//printf(":%s\n", msg);
						//printf("[REV] %d bytes from <LOC:%d> <TYPE:%s>\n", msgLen, recvLoc, msgType);
						printf("***** Port %d move to location %d *****\n", currPortNum, loc);
						// Send immediately after move
						msg_store_send(msgStore, store_idx, &store_len, sd, server_address, location, portNumber, ipAddress, numHosts, locIdx, row, col);
					}
					else {
						for (i = 0; i < numHosts; i++) {
							if (toPortNum == *portNumber[i]) {
								*location[i] = atoi(msg);
								break;
							}
						}
					}
				}
				/* Resend MSG & ACK if in range */
				else if (check_distance(loc, recvLoc, row, col, DISTANCE) && hopCount > 0) {
					hopCount--;

					/* MSG or ACK for me */
					if (toPortNum == currPortNum) {	
						// ACK for me -> delete
						printf("[RCV] %d bytes from <LOC:%d> <TYPE:%s>\n", msgLen, recvLoc, msgType);
						printf(":%s\n", msg);
						if (!strcmp(msgType, "ACK")) { 
							for (i = 0; i < MAX_STORE; i++) {
								if (*store_idx[i] != -1) {
									// buffer = "<ver> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>"
									sscanf(msgStore[i].msg, "%d %d %d %d %d %s %d %s %[^\n]s", &t_v, &t_loc, &t_to, &t_from, &t_hop, t_type, &t_id, t_route, t_msg);
									if (t_id == recvMsgID && t_to == fromPortNum && t_from == toPortNum)
										msg_store_delete(msgStore, store_idx, &store_len, i);
									printf("##### Deleting ack-ed msg @ slot %d #####\n", i);
								}
							}
						}
						// MSG for me -> send ACK
						else {
							// Create buffer
							hopCount = HOPCOUNT;
							memset(msgType, '\0', sizeof(msgType));
							sprintf(msgType, "%s", "ACK");
							memset(route, '\0', sizeof(route));
							sprintf(route, "%d", currPortNum);
							memset(msg, '\0', sizeof(msg));
							sprintf(msg, "%s", "(ACK)");
							memset(buffer, '\0', sizeof(buffer));
							sprintf(buffer, "%d %d %d %d %d %s %d %s %s", VERSION, loc, fromPortNum, toPortNum, HOPCOUNT, msgType, recvMsgID, route, msg);
							msgLen = strlen(msg);
							
							// Send ACK 
							for (i = 0; i < numHosts; i++) {
								if (fromPortNum == *portNumber[i]) {
									toLoc = *location[i];
									toIdx = i;
								}
							}
							if (check_distance(loc, toLoc, row, col, DISTANCE)) {
							 	send_msg(buffer, sd, server_address, loc, *portNumber[toIdx], ipAddress[toIdx]);
								printf("[ACK] to <LOC:%d>\n", toLoc);
							}
						 	else 
								msg_store_write(msgStore, store_idx, &store_len, buffer, fromPortNum, route);
							// 	for (i = 0; i < numHosts; i++) {
							// 		if (i == locIdx) continue;
							// 		if (!check_distance(loc, *location[i], row, col, DISTANCE)) continue;
							// 		send_msg(buffer, sd, server_address, *location[i], *portNumber[i], ipAddress[i]);
							// 	}
							// }
						}
					}
					/* MSG or ACK for others*/
					else {	
						// // ACK, check if my msg is ACKed
						if (!strcmp(msgType, "ACK")) {
							for (i = 0; i < MAX_STORE; i++) {
								if (*store_idx[i] != -1) {
									// buffer = "<ver> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>"
									sscanf(msgStore[i].msg, "%d %d %d %d %d %s %d %s %[^\n]s", &t_v, &t_loc, &t_to, &t_from, &t_hop, t_type, &t_id, t_route, t_msg);
									if (t_id == recvMsgID && t_to == fromPortNum && t_from == toPortNum) {
										msg_store_delete(msgStore, store_idx, &store_len, i);
										printf("##### Deleting ack-ed msg @ slot %d #####\n", i);
									}
								}
							}
						}
						// Add my port to the msg
						sprintf(portBuffer, "%d", currPortNum); // change int to str
						if (!strstr(route, portBuffer)) {
							sprintf(newRoute, "%s,%s", route, portBuffer);
							sprintf(buffer, "%d %d %d %d %d %s %d %s %s", VERSION, loc, toPortNum, fromPortNum, hopCount, msgType, recvMsgID, newRoute, msg);
							for (i = 0; i < numHosts; i++) {
								if (toPortNum == *portNumber[i])
									toIdx = i;
							}
							msg_store_write(msgStore, store_idx, &store_len, buffer, toPortNum, route);
						}
					}	
				}
			}
		}
	}

	/*************
	 *8* Exiting *
	 *************/
	close(sd);
	printf("\n[Exiting]..\n");
	return 0;
}
/**main ends**/


void print_block(char* title)
{
	/* 
	int len = strlen(title);
	int i = 0;
	for(i = 0; i < len + 4; i++)
		printf("#");
	printf("\n# %s #\n", title);
	for(i = 0; i < len + 4; i++)
		printf("#");
	printf("\n");
	*/
	printf("===== %s =====\n", title);
}


void send_msg(char* buffer, int sd, struct sockaddr_in server_address, int loc, int port, char* ipAddr)
{
	/* Update loc field */
	int t_ver, t_loc;
	char t_msg[MSG_LEN];
	sscanf(buffer, "%d %d %[^\n]s", &t_ver, &t_loc, t_msg);
	sprintf(buffer, "%d %d %s", t_ver, loc, t_msg);
	/* Send msg */
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = inet_addr(ipAddr);
	int rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	if (rc < 0) { close(sd); perror("sendto"); exit(-1); }	
	//printf("[SNT] to <LOC:%d> @ <%s:%d>\n", loc, ipAddr, port);
}


int get_msg_type(char* msgType)
{
	int rc;
	if (!strcmp(msgType, "MSG")) rc = MSG;
	if (!strcmp(msgType, "ACK")) rc = ACK;
	if (!strcmp(msgType, "MOV")) rc = MOV;
	return rc;
}


void msg_store_write(MsgStore msgStore[], int* store_idx[], int* store_len, char* buffer, int toPortNum, char* route) 
{
	int i;
	int existed = 0;
	for (i = 0; i < MAX_STORE; i++) {
		if (*store_idx[i] != -1) {
			if (!strcmp(buffer, msgStore[i].msg)) {
				existed = 1;
				break;
			}
		}
	}
	if (!existed) {
		for (i = 0; i < MAX_STORE; i++) {
			if (*store_idx[i] == -1) {
				msgStore[i].resend = RESEND;
				sprintf(msgStore[i].msg, "%s", buffer);
				//printf("saving: %s\n", buffer);
				msgStore[i].toPortNum = toPortNum;
				sprintf(msgStore[i].route, "%s", route);
				*store_idx[i] = 1;
				*store_len += 1;
				break;
			}
		}
		printf("##### Saving in slot %d #####\n", i);
	}
}


void msg_store_send(MsgStore msgStore[], int* store_idx[], int* store_len, int sd, struct sockaddr_in server_address, int* location[], int* portNumber[], char* ipAddress[], int numHosts, int locIdx, int row, int col)
{
	int i, j;
	char buff[MSG_LEN];
	for (i = 0; i < MAX_STORE; i++) {
		if (*store_idx[i] != -1) {
			(msgStore[i].resend)--;
			int toIdx = -1;
			for (j = 0; j < numHosts; j++) {
				if (*portNumber[j] == msgStore[i].toPortNum)
					toIdx = j;
			}

			if (msgStore[i].resend >= 0) {
				/* Send msg */
				if (check_distance(*location[locIdx], *location[toIdx], row, col, DISTANCE)) {
					send_msg(msgStore[i].msg, sd, server_address, *location[locIdx], *portNumber[toIdx], ipAddress[toIdx]);
					//printf("[SNT] to <LOC:%d> %d-Tries\n", *location[toIdx], msgStore[i].resend);
				}
				else {
					for (j = 0; j < numHosts; j++) {
						sprintf(buff, "%d", *portNumber[j]); // change int to str
						if (j == locIdx || strstr(msgStore[i].route, buff)) continue;
						if (!check_distance(*location[locIdx], *location[j], row, col, DISTANCE)) continue;
						send_msg(msgStore[i].msg, sd, server_address, *location[locIdx], *portNumber[j], ipAddress[j]);
					}
					//printf("%s\n", msgStore[i].msg);
					//printf("[BRC] from <LOC:%d> %d-Tries\n", *location[locIdx], msgStore[i].resend);
				}
			}
			else {
				msg_store_delete(msgStore, store_idx, store_len, i);
				printf("##### Deleting slot %d after 5-tries #####\n", i);
			}
		}		
	}
}



void msg_store_delete(MsgStore msgStore[], int* store_idx[], int* store_len, int i)
{
	memset(msgStore[i].msg, '\0', sizeof(msgStore[i].msg));
	*store_idx[i] = -1;
	*store_len -= 1;
}
