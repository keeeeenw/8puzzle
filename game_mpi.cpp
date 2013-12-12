/* 
 * 8 Puzzle Solver using MPI
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
#include <sys/time.h>	// for gettimeofday
#include "mpi.h"        // for MPI routines, definitions, etc 
using namespace std;

#include "game.h"

#define INF 1000000				/* Proxy for inifinite */
#define COMM_INTERVAL 0.0000001		/* Time between communication steps */
#define MASTER 0				/* ID of the master node */
#define TERMINATION 1			/* Tags termination messages */
#define Token 2					/* Tags token message */
#define UNEXAMINED_SUBPROBLEM 3 /* Tags message containing unexamined subproblem */

void printToken(struct TOKEN *token)
{
	printf("-------- Token Start ------------\n");
	printf("cost %d \n", token->c);
	printf("count %d \n", token->count);
	if(token->color == WHITE)
		printf("Token Color White \n");
	else
		printf("Token Color Black \n");
	printState(&(token->s)); 
	printf("-------- Token End ------------\n");
}


int main(int argc, char *argv[])
{
	int n = 3; //board size is 3x3

	int myRank, numProcs;						// MPI related variables
	double start_t = 0, end_t = 0, total_t = 0; //time evaluation

	// initialize MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
	MPI_Status status;
                
    // setup random seed
    srand(time(NULL));

	// declare and initialize cost variables
	int global_c, local_c;				// cost of global and local best solution so far
	local_c = INF;
	global_c = INF;	

	// declare and initialize halt variables
	enum Color color;					// process color (for termination detection)
	color = WHITE;
	int msgCount;						// messages sent minus messages received
	msgCount = 0;

	// token passed around ring for termination detection
	struct TOKEN *token	= (TOKEN*)malloc(sizeof(struct TOKEN));	

	priority_queue<state> queue;		// priority queue, each process has its own queue

	/********************** USE TIME TO CHECK TOKEN **********************/
	timeval lastComm;				// time of last communication
	gettimeofday(&lastComm, NULL);	// use gettimeofday to obtain time

	//double last_comm;					// time of last communication
	//last_comm = MPI_Wtime();			// use MPI library to obtain time
	/*********************************************************************/

	/********************* USE COUNTER TO CHECK TOKEN ********************/
	int ic;
	ic = 0;
	/*********************************************************************/

	// initial state of the board
	struct state *initial = (state*)malloc(sizeof(struct state));

    // initialize local_bestState (best solution)
	struct state *local_bestState = (state*)malloc(sizeof(struct state));
	int *local_board1;
	local_board1 = (int*)malloc(n*n * sizeof(int));
	setState(local_bestState, local_board1, n, 0);
	local_bestState->lowerBound = INF;

	//initialize currentState
	struct state *currentState = (state*)malloc(sizeof(struct state));
	int *local_board2;
	local_board2 = (int*)malloc(n*n * sizeof(int));
	setState(currentState, local_board2, n, 0);
	currentState->lowerBound = INF;

	//initialize nextState	
	struct state *nextState = (state*)malloc(sizeof(struct state));
	int *local_board3;
	local_board3 = (int*)malloc(n*n * sizeof(int));
	setState(nextState, local_board3, n, 0);
	nextState->lowerBound = INF;

	// initilization code execute by master node
	if(myRank == MASTER) 
	{
		//create board
		int *board;
		board = (int*)malloc(n*n * sizeof(int));

		// easy board from book (6 steps)
		/*
		board[0] = 1;
		board[1] = 5;
		board[2] = 2;
		board[3] = 4;
		board[4] = 3;
		board[5] = 0;
		board[6] = 7;
		board[7] = 8;
		board[8] = 6;
		*/

		// medium board (183869 steps)
		board[0] = 1;
		board[1] = 0;
		board[2] = 3;
		board[3] = 4;
		board[4] = 5;
		board[5] = 2;
		board[6] = 7;
		board[7] = 6;
		board[8] = 8;

		// hard board (4007316 steps)
		//board[0] = 8;
		//board[1] = 5;
		//board[2] = 3;
		//board[3] = 4;
		//board[4] = 7;
		//board[5] = 6;
		//board[6] = 1;
		//board[7] = 0;
		//board[8] = 2;

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

		/********************** CHECK SOLVABILITY **********************/
		// check solvability
		/*while(!isSolvable(board, n)){
			shuffleBoard(board, n*n);
		}*/
		/***************************************************************/

		// START THE TIMER
		start_t = MPI_Wtime();

		// set initial state
		setState(initial, board, n, 0);

		#ifdef DEBUG
		printf("Initial \n");
		printBoard(initial->board, n);
		#endif

		//push the state to the queue
		queue.push(*initial);

		//initialize token
		token->c = INF;
		token->color = WHITE;
		token->count = 0;
		token->s = *initial;

		// send token to next process (myRank = 1)
		msgCount++;
		color = BLACK;
		// initialize send buffer
		int *initialPackedToken = (int*)malloc((n*n+6) * sizeof(int));
		// pack token
		packToken(token, initialPackedToken);
		// send token to the successor
		MPI_Send(initialPackedToken, n*n+6, MPI_INT, 1, Token, MPI_COMM_WORLD);
		// free send buffer
		free(initialPackedToken);
		color = WHITE;
	}

	while(true)
	{
		// if use time to ckeck token
		//if(queue.empty() || timeDiff(&lastComm) > COMM_INTERVAL)
		//if(queue.empty() || MPI_Wtime()-last_comm > COMM_INTERVAL)
		// if use counter to check token
		if(queue.empty() || ic % 20 ==0)
		{
			/****************************** BandB_Communication() Start ******************************/
			// initialize pending message flags
			int terminationFlag = 0, tokenFlag = 0, unexaminedSubFlag = 0;
			// get the neighbors in the ring (receive from left, send to right)
			int leftNeighbor = (myRank + numProcs - 1) % numProcs;
			int rightNeighbor = (myRank + 1) % numProcs;

			/*********************** handle TERMINATION tag ***********************/
			// check pending message with TERMINATION tag
			MPI_Iprobe(MASTER, TERMINATION, MPI_COMM_WORLD, &terminationFlag, &status);
			if(terminationFlag!=0)
			{
				//termination receive
				MPI_Recv(0, 0, MPI_INT, MASTER, TERMINATION, MPI_COMM_WORLD, &status);

				//Halt the process
				#ifdef DEBUG
				printf("node %d, termination flag recieved \n", myRank);
				#endif

				MPI_Finalize(); 
				return 0;
			}

			/*********************** handle Token tag ***********************/
			// check pending message with Token tag
			MPI_Iprobe(leftNeighbor, Token, MPI_COMM_WORLD, &tokenFlag, &status);
			if(tokenFlag!=0)
			{
				// initialize receive buffers
				int *unPackedToken = (int*)malloc((n*n+6) * sizeof(int));
				// receive token from previous node
				MPI_Recv(unPackedToken, n*n+6, MPI_INT, leftNeighbor, Token, 
					MPI_COMM_WORLD, &status);
				// unpack and reconstruct token
				unpackToken(unPackedToken, token);
				free(unPackedToken);

				if(myRank != MASTER)
					msgCount--;

				// update token cost if local cost is smaller
				if(local_c < token->c)
				{
					token->c = local_c;
					token->s = *local_bestState;
				}

				// clear queue if best solution in queue is worse than token cost
				if(!queue.empty())
				{
					struct state *tempState  = (state*)malloc(sizeof(struct state));
					*tempState = queue.top();
					if(token->c <= tempState->lowerBound)
					{
						queue = priority_queue<state>(); 
					}
				}
				
				// set global cost to token cost
				global_c = token->c;

				// HEAD receive token, node check termination
				if(myRank == MASTER)
				{
					#ifdef DEBUG
					printf("*********** Master Received Token Start *************** \n");
					printf("msgCount %d \n", msgCount);
					printToken(token);
					printf("*********** Master Received Token End *************** \n");
					#endif

					if( color == WHITE 
						&& token->color == WHITE 
						&& token->count + msgCount == 0)
					{ 
						// stop timer
						end_t = MPI_Wtime();
						// record time
						total_t = end_t - start_t;

						#ifdef DEBUG
						printf("Sending Termination Tag, Stop Master \n");
						#endif

						// send out message with TERMINATION tag to other nodes 
						int rank;
						for (rank = 1; rank < numProcs; rank++) {
							MPI_Send(0, 0, MPI_INT, rank, TERMINATION, MPI_COMM_WORLD);
						}

						#ifdef DEBUG
						printf("+++++++++++++++++ Solution ++++++++++++++++++++ \n");
						printToken(token);
						printf("Total Time: \n");
						#endif

						// print time
						printf("%d \t %f \n", numProcs, total_t);

						MPI_Finalize(); //terminate itself
						return 0;
					}
					else
					{
						token->color = WHITE;
						token->count = 0;
					}
				}
				else // change the color of token black if the node is black
				{
					if(color == BLACK)
						token->color = BLACK;
					token->count = token->count+msgCount;
				}

				// forward token to the successor
				msgCount++;
				color = BLACK;

				// initialize send buffer
				int *packedToken = (int*)malloc((n*n+6) * sizeof(int));
				// pack token
				packToken(token, packedToken);
				// send token to the successor
				MPI_Send(packedToken, n*n+6, MPI_INT, rightNeighbor, Token, MPI_COMM_WORLD);
				// free buffer
				free(packedToken);
				//after sending message
				color = WHITE; 
			}

			/*********************** handle Unexamined_Subproblem tag ***********************/
            #ifdef DEBUG
            //printf("node %d probing for unexamined subproblem \n", myRank);
            #endif 
			//MPI_Iprobe(leftNeighbor, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
			MPI_Iprobe(MPI_ANY_SOURCE, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
			while(unexaminedSubFlag!=0)
			{
                #ifdef DEBUG
                printf("node %d receives unexamined problem \n", myRank);
                #endif 

				// initialize receive buffers
				int *unPackedState = (int*)malloc((n*n+3) * sizeof(int));
				struct state *tempRecvState = (state*)malloc(sizeof(struct state));

				MPI_Recv(unPackedState, n*n+3, MPI_INT, MPI_ANY_SOURCE, UNEXAMINED_SUBPROBLEM, 
					MPI_COMM_WORLD, &status);
				
				// unpack receive problem and create new state
				unpackState(unPackedState, tempRecvState);
				// free buffer
				free(unPackedState);

				msgCount = msgCount - 1;
				color = BLACK;
				if(tempRecvState->lowerBound < global_c)
				{
					queue.push(*tempRecvState);
				}

				// reset tag to continue receiving sub problem
				unexaminedSubFlag = 0;
				MPI_Iprobe(MPI_ANY_SOURCE, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
			}

			/*********************** handle subproblem in q **************************/
			// if more than one unexamined subproblem in queue, then
            #ifdef DEBUG
            if(myRank == MASTER)
                printf("node %d, queue size %lu \n", myRank, queue.size());
            #endif 
			if(queue.size() > 1)
			{
                #ifdef DEBUG
                printf("node %d has unexamined problem \n", myRank);
                #endif 

				struct state *tempSendState = (state*)malloc(sizeof(struct state));
				*tempSendState = queue.top();
				queue.pop();

				// initialize pack buffer
				int *packedState = (int*)malloc((n*n+3) * sizeof(int));
				packState(tempSendState, packedState);
				
				//MPI_Ssend(packedState, n*n+3, MPI_INT, rightNeighbor, UNEXAMINED_SUBPROBLEM, 

                // Send it to a random node
                int node = rand() % numProcs;
                while (node == myRank)
                {
                    node = rand() % numProcs;
                }

                #ifdef DEBUG
                printf("node %d, sending to node %d \n", myRank, node);
                #endif 

				MPI_Send(packedState, n*n+3, MPI_INT, node, UNEXAMINED_SUBPROBLEM, 
					MPI_COMM_WORLD);

				// free pack buffer
				free(packedState);

				msgCount = msgCount + 1;
				color = BLACK;
			}

			/****************************** BandB_Communication() End ******************************/
			// if use time to check token
			gettimeofday(&lastComm, NULL); //get current time
			//last_comm = MPI_Wtime();
			// if use counter to check token
		}
		else if(!queue.empty()) //distribute the work
		{
			*currentState = queue.top();
			queue.pop();

			#ifdef DEBUG
            printf("queue not empty on node %d, queue size: %lu \n", myRank, queue.size());
			//printState(currentState);
			#endif 

			if(currentState->lowerBound < local_c )
			{
				color = BLACK;
				// if currentState is the solution
				if(checkResult(currentState->board, currentState->dim))
				{
					if(currentState->lowerBound < global_c)
					{
						*local_bestState = *currentState;
						local_c = currentState->lowerBound;
					}
				}
				else
				{
					int i, k;
					int holeCol, holeRow;

					// Find out the location of the hole
					for(i=0; i<currentState->dim*currentState->dim; i++){
						if(currentState->board[i]==0){
							holeRow = i / currentState->dim;
							holeCol = i % currentState->dim;
						}
					}

					// Find directions of the states based on the location of the hole
					int numDirections = 4; //at most 4 directions
					int *directions = (int*)malloc(numDirections * sizeof(int));
					numDirections = setHoleDirection(directions, holeRow, holeCol, currentState->dim);

					// For each direction, find the nextState
					for(i=0; i<numDirections; i++)
					{
						k = directions[i];
						struct state *nextState = (state*)malloc(sizeof(struct state));
						makeAState(k, currentState, nextState); //this is v in the pseudo code

						#ifdef DEBUG
						//printf("node %d, global_c %d \n", myRank, global_c);
						//printState(nextState);
						#endif

						if(nextState->lowerBound < global_c)
						{
							#ifdef DEBUG
							//printf("node %d, next state: \n", myRank);
							//printState(nextState);
							#endif
							queue.push(*nextState);
						}
					}
					free(directions);
				}
			}
		}
        ic++;
        #ifdef DEBUG
        //printf("node %d, queue size %lu \n", myRank, queue.size());
        //if(ic % 20 == 0)
        //    printf("ic: %d \n", ic);
        #endif

	}
}
