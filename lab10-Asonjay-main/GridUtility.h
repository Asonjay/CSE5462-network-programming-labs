/*****************************************************************************\
 * Title:                                                                    *
 *	 GridUtility.h                                                           *
 * Author:                                                                   *
 *	 Jason Xu (xu.2460)                                                      *
 * Description:                                                              *
 *	 Includes utilities functions for grid implementaion                     *
 * Function signature:                                                       *
 *   void readGridData(int row, int col, int currLoc, int DISTANCE);         *
 *   void getCords(int grid_row, int grid_col, int* x, int* y, int loc);     *
 *   int checkDistance(int currLoc, int tarLoc, int row, int col, int dist); *
\*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "DefineValue.h"

void read_grid_data(int row, int col, int currLoc);
void get_cords(int grid_row, int grid_col, int* x, int* y, int loc);
int check_distance(int currLoc, int tarLoc, int row, int col, int dist);

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
void read_grid_data(int row, int col, int currLoc) 
{
	int currRow;
	int currCol;
	// Calculate curr row & col
	get_cords(row, col, &currRow, &currCol, currLoc);

	/*
	int i;
	int j;
	printf("(***): current location\n");

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
	printf("DISTANCE set: %d\n", DISTANCE);
	*/
} /* read_grid_data */


/***************************************************************
 * Calculate row and col based on given grid size and location *
 * Notes:                                                      *
 *   (7, 6) 25(4, 0) -> (25 / 6 = 4, 25 % 6 = 1)               *
 *	 (7, 6) 24(3, 5) -> (24 / 6 = 4 - 1, 6 - 1 = 5)            *
 ***************************************************************/
void get_cords(int grid_row, int grid_col, int* x, int* y, int loc) {
	if (loc % grid_col == 0) {
		*x = loc / grid_col - 1;
		*y = grid_col - 1;
	} 
	else {
		*x = (int) (loc / grid_col);
		*y = loc % grid_col - 1;
	}
} /* get_cords */


/*******************************************************************
*	Check if tarLoc is within the distance of currLoc with Euc_Dist
* 	:return:
*		(tarLoc is within the distance of currLoc)
*******************************************************************/
int check_distance(int currLoc, int tarLoc, int row, int col, int dist) {
	int curr_x;
	int curr_y;
	int tar_x;
	int tar_y;
	get_cords(row, col, &curr_x, &curr_y, currLoc);
	get_cords(row, col, &tar_x, &tar_y, tarLoc);
	return (int) sqrt((double)(pow(curr_x - tar_x, 2) + pow(curr_y - tar_y, 2))) <= dist;
} /* check_distance */



