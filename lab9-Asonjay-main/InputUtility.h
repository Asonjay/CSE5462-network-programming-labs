/*******************************************************************************\
 * Title:                                                                      *
 *	 InputUtility.h                                                            *
 * Author:                                                                     *
 *	 Jason Xu (xu.2460)                                                        *
 * Description:                                                                *
 *	 Includes utilities functions for reading inputs from pre-configured       *
 *   config.txt file                                                           *
 * Function signature:                                                         *
 *   int readConfigFile(char* ipAddress[], int* portNumber[], int* location[], * 
 *                      int MAX_ENTRY, int BUFFER_LEN);                        *
 *   int isValidIpAddr (char* ipaddr);                                         *
 *   void printInputErrorMsg(char* msg);                                       *
\*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

int read_config_file(char* ipAddress[], int* portNumber[], int* location[], int* msgID[], int* row, int* col, int MAX_ENTRY, int BUFFER_LEN);
int is_valid_ip_addr(char* ipaddr);  
void print_input_error_msg(char* msg); 

/*****************************************************	
 * This function will read the entry from config.txt *
 * and validate the format.                          *
 * :return:                                          *
 *	 number of entries                               *
 *****************************************************/
int read_config_file(char* ipAddress[], int* portNumber[], int* location[], int* msgID[], int* row, int* col, int MAX_ENTRY, int BUFFER_LEN) 
{	
	// Writing entry in to pointers
	int i;
	for (i = 0; i < MAX_ENTRY; i++) {
        if ((ipAddress[i] = malloc(sizeof(char) * BUFFER_LEN)) == NULL) 
			{ perror("malloc"); return -1; }
        if ((portNumber[i] = malloc(sizeof(int) * BUFFER_LEN)) == NULL) 
			{ perror("malloc"); return -1; } 
        if ((location[i] = malloc(sizeof(int) * BUFFER_LEN)) == NULL)
			{ perror("malloc"); return -1; }
		if ((msgID[i] = malloc(sizeof(int) * BUFFER_LEN)) == NULL)
			{ perror("malloc"); return -1; } 
	}
	FILE *fp;
	char* end;
	char dataBuffer[BUFFER_LEN];
	int count = 0;
	int setNum = 0;
	// Reading from config.txt
	memset(dataBuffer, '\0', sizeof(dataBuffer));
	fp = fopen("./config.txt", "r");   
	while (fscanf(fp, "%s", dataBuffer) != EOF) {
		if (count == 0) 
			*row = strtol(dataBuffer, &end, 10);
		else if (count == 1) 
			*col = strtol(dataBuffer, &end, 10);
		else {
			setNum = (int)((count - 2) / 3.0);
			if ((count - 2) % 3 == 0) {
				if (!is_valid_ip_addr(dataBuffer)) print_input_error_msg("[Error] invalid <ipaddr>\nCheck config.txt, Usage is: <ipaddr> <portNumber> <location>\n");
				strcpy(ipAddress[setNum], dataBuffer);			
			} 
			else if ((count - 2) % 3 == 1) {
				*portNumber[setNum] = strtol(dataBuffer, &end, 10);
				if (end == dataBuffer) print_input_error_msg("[Error] invalid <portNumber>\nCheck config.txt, Usage is: <ipaddr> <portNumber> <location>\n");
				*msgID[setNum] = 1;
			} 
			else {
				*location[setNum] = strtol(dataBuffer, &end, 10);
				if (end == dataBuffer) print_input_error_msg("[Error] invalid <location>\nCheck config.txt, Usage is: <ipaddr> <portNumber> <location>\n");
			}
		}
		count++;
	}
	fclose(fp);
	return (count - 2) / 3;
} /* read_config_file */


/***************************************************************
 * This function will check if a string is valid IPv4 address. *
 * :return:                                                    *
 *   (<ipaddr> is valid)                                       *
 ***************************************************************/
int is_valid_ip_addr(char *ipaddr){
	struct sockaddr_in sockaddr;
	int result = inet_pton(AF_INET, ipaddr, &(sockaddr.sin_addr));
	return result == 1;
} /* is_valid_ip_addr */


/*********************************************************************
 * Print error msg from invalid usage of command line and config.txt *
 * :Error code:                                                      *
 *   exit(1)                                                         *
 *********************************************************************/
void print_input_error_msg(char* msg){
	printf(msg);
	exit(1);
} /* printInputErrorMsg */
