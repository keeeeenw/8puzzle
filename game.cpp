/*
 * 8 Puzzle Solver using MPI (serial version for comparison)
 * Using Pseudo-code from "Parallel Programming in C with MPI and OpenMP" by Michael Quinn
 * Pseudo-code can be found on page 389 - 390, note that multiple errors were found in it
 * Author: Yu Zhao (Meow Meow) and Zixiao Wang (Ken)
 * Date: December 2013
*/

#include <stdio.h>		// for printf
#include <stdlib.h>		// for malloc
#include <string.h>		// for memcpy
#include <iostream>		// for CPP
#include <queue>		// for priority queue
#include <time.h>
using namespace std;

#include "game.h"

#define INF 1000000				/* Proxy for inifinite */

int main(int argc, char *argv[])
{
	int n = 3;				//board size is 3x3
	int i, j=0;				// iterator
	double elapsedTime;
	timeval start, end;		// time of last communication

	// declare initial state (game start)
	struct state *initial = (state*)malloc(sizeof(struct state));

	// initialize solution
	struct state *solution = (state*)malloc(sizeof(struct state));
	int *local_board1;
	local_board1 = (int*)malloc(n*n * sizeof(int));
	setState(solution, local_board1, n, 0);
	solution->lowerBound = INF;

	// initialize currentState
	struct state *currentState  = (state*)malloc(sizeof(struct state));
	int *local_board2;
	local_board2 = (int*)malloc(n*n * sizeof(int));
	setState(currentState, local_board2, n, 0);
	currentState->lowerBound = INF;

	// initialize nextState
	struct state *nextState = (state*)malloc(sizeof(struct state));
	int *local_board3;
	local_board3 = (int*)malloc(n*n * sizeof(int));
	setState(nextState, local_board3, n, 0);
	nextState->lowerBound = INF;

	priority_queue<state> queue;

	//create board
	int *board;
	board = (int*)malloc(n*n * sizeof(int));

	// easy board from book (6 steps)
	board[0] = 1;
	board[1] = 5;
	board[2] = 2;
	board[3] = 4;
	board[4] = 3;
	board[5] = 0;
	board[6] = 7;
	board[7] = 8;
	board[8] = 6;

	// medium board (183869 steps)
	/*
	board[0] = 1;
	board[1] = 0;
	board[2] = 3;
	board[3] = 4;
	board[4] = 5;
	board[5] = 2;
	board[6] = 7;
	board[7] = 6;
	board[8] = 8;
	*/

	// hard board (4007316 steps)
	board[0] = 8;
	board[1] = 5;
	board[2] = 3;
	board[3] = 4;
	board[4] = 7;
	board[5] = 6;
	board[6] = 1;
	board[7] = 0;
	board[8] = 2;

	// solution for check
	/*
	board[0] = 1;
	board[1] = 2;
	board[2] = 3;
	board[3] = 4;
	board[4] = 5;
	board[5] = 6;
	board[6] = 7;
	board[7] = 8;
	board[8] = 0;
	*/

	//building board randomly
	//fillBoard(board, n);
	//shuffleBoard(board, n*n);	

	/********************** CHECK SOLVABILITY **********************/
	// check solvability
	/*while(!isSolvable(board, n)){
		shuffleBoard(board, n*n);
	}*/
	/***************************************************************/

	// START THE TIMER
	gettimeofday(&start, NULL);

	// set initial state
	setState(initial, board, n, 0);

	#ifdef DEBUG
    printf("Problem Starts. Here is our board: \n");
	printBoard(initial->board, n);
	printf("lowerBound is %d \n", initial->lowerBound);
	#endif

	//push the state to the queue
	queue.push(*initial);

	////while (!queue.empty())


	while (!queue.empty())
	{
		*currentState = queue.top();
		queue.pop();

		if(checkResult(currentState->board, n)) //finished
		{
			*solution = *currentState;

			//#ifdef DEBUG
			printf("****************************** \n");
			printf("The initial problem is \n");
			printBoard(initial->board, n);
			printf("The solution of the problem is \n");
			printBoard(solution->board, n);
			//#endif

			// stop the timer and record time
			gettimeofday(&end, NULL);
			elapsedTime = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
			elapsedTime = elapsedTime / 1000000;
			printf("%f \n", elapsedTime);
			break;
		}
		else
		{
			int mininumBound = 10000, i, k, chosenDirection;
			int holeCol, holeRow;

			// Find out the location of the hole
			for(i=0; i<n*n; i++){
				if(currentState->board[i]==0){
					holeRow = i / n;
					holeCol = i % n;
				}
			}

			// Find directions of the moves based on the location of the hole
			int numDirections = 4; //at most 4 directions
			int *directions = (int*)malloc(numDirections * sizeof(int));
			numDirections = setHoleDirection(directions, holeRow, holeCol, n);

			j++;
			#ifdef DEBUG
			if(j % 100 == 0){
				printf("++++++++++ iteration %d ++++++++++++\n", j);
				printf("Current Board: \n");
				printf("lowerBound is %d and moveSoFar is %d \n", currentState->lowerBound, currentState->moveSoFar);
				printBoard(currentState->board, n);
			}
			#endif

			// For each direction, find the nextState
			for(i=0; i<numDirections; i++)
			{
				k = directions[i];
				makeAState(k, currentState, nextState);

				#ifdef DEBUG
				printf("Info on next possible move \n");
				printBoard(nextState->board, n);
				printf("move direction %d, lowerboud %d \n", k, nextState->lowerBound);
				printf("move direction %d, movesofar %d \n", k, nextState->moveSoFar);
				printf("move direction %d, Manhattan total %d \n", k, getBoardManhattan(nextState->board, n));
				#endif

				queue.push(*nextState);
			}
			free(directions);
		}
	}
	freeState(nextState);
	freeState(solution);
	// ????? not allow to free currentState, why?
	//freeState(currentState);
	freeState(initial);
	
	return 0;
}
