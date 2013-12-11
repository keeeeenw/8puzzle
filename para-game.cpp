/*
 *
 * Author: Yu Zhao and Zixiao Wang
 *
 *
 * Some questions:
 *  1. How to halt a process
 *      MPI_Finalize()?
 *  2. How to check pending messeage?
 *      MPI_Iprobe(MASTER, TERMINATION, MPI_COMM_WORLD, &terminationFlag, &status);
 *  3. Do we need to recieve the messsage after checking
 *  4. How to check whether a message contains a tag or token
 *      
 *
 *
 *
 *
 *
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
#define COMM_INTERVAL 50   	    /* Time between communication steps */
#define MASTER 0				/* ID of the master node */
#define TERMINATION 1			/* Tags termination messages */
#define Token 2					/* Tags token message */
#define UNEXAMINED_SUBPROBLEM 3 /* Tags message containing unexamined subproblem */

void printToken(struct TOKEN *token)
{
    printf("-------- Token Start ------------\n");
    printf("cost %d \n", token->c);
    printf("count %d \n", token->count);
    printf("-------- Token End ------------\n");
}

int main(int argc, char *argv[])
{
	int myRank, numProcs;				// MPI related variables

	// initialize MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

	//printf("node %d here 1 \n", myRank);

    // initialize variables
	enum Color color;					// process color (for termination detection)
	int global_c, local_c;				// cost of global and local best solution so far
	struct timeval lastComm;			// time of last communication
	int msgCount;						// messages sent minus messages received
	priority_queue<state> queue;		// priority queue, each process has its
                                        // own queue

	MPI_Status status;

	// token passed around ring for termination detection
	struct TOKEN *token	= (TOKEN*)malloc(sizeof(struct TOKEN));	

    // initial state of the board
    struct state *initial = (state*)malloc(sizeof(struct state));

	int n = 3; //board size is 3x3

	if(myRank == MASTER) //master initialize board
	{
		int *board;
		board = (int*)malloc(n*n * sizeof(int));

		/*// fill board randomly
		srand(time(NULL));
		fillBoard(board, n);
		shuffleBoard(board, n*n);*/

		// fill specific board 
		board[0] = 1;
		board[1] = 5;
		board[2] = 2;
		board[3] = 4;
		board[4] = 3;
		board[5] = 0;
		board[6] = 7;
		board[7] = 8;
		board[8] = 6;

		//printf("node %d here 2 \n", myRank);

		// check solvability
		//while(!isSolvable(board, n)){
		//	shuffleBoard(board, n*n);
		//}

		// construct initial state
		setState(initial, board, n, 0);

		queue.push(*initial);
		token->c = INF;
		token->color = WHITE;
		token->count = 0;
        token->s = *initial;

        // initialize send buffer
        int *initialPackedToken = (int*)malloc((n*n+6) * sizeof(int));
        // pack token
        packToken(token, initialPackedToken);
        // send token to the successor
        MPI_Send(initialPackedToken, n*n+6, MPI_INT, 1, Token, MPI_COMM_WORLD);
        // free send buffer

        free(initialPackedToken);
	}

	// different states
	struct state *local_bestState = (state*)malloc(sizeof(struct state));
	struct state *currentState  = (state*)malloc(sizeof(struct state));
	struct state *nextState = (state*)malloc(sizeof(struct state));

    //initialize local_bestState (best solution)
    int *local_board1;
    local_board1 = (int*)malloc(n*n * sizeof(int));
	setState(local_bestState, local_board1, n, 0);
	local_bestState->lowerBound = INF;

    //initialize currentState
    int *local_board2;
    local_board2 = (int*)malloc(n*n * sizeof(int));
	setState(currentState, local_board2, n, 0);
	currentState->lowerBound = INF;

    //initialize nextState
    int *local_board3;
    local_board3 = (int*)malloc(n*n * sizeof(int));
	setState(nextState, local_board3, n, 0);
	nextState->lowerBound = INF;

	local_c = INF;
	gettimeofday(&lastComm, NULL);
	msgCount = 0;
	color = WHITE;

    int i = 0;
    for (int i=0; i<1000000; i++) //this suppose to repeat forever
    {
        int timediff = timeDiff(&lastComm);
        if(queue.empty() || timediff > COMM_INTERVAL)
        {
            
            /****************************** BandB_Communication() ******************************/
            int terminationFlag = 0, tokenFlag = 0, unexaminedSubFlag = 0;
            // get the neighbors in the ring (receive from left, send to right)
            int leftNeighbor = (myRank + numProcs - 1) % numProcs;
            int rightNeighbor = (myRank + 1) % numProcs;

            // check pending message with TERMINATION tag
            MPI_Iprobe(MASTER, TERMINATION, MPI_COMM_WORLD, &terminationFlag, &status);
            if(terminationFlag!=0)
            {
                // ????? do we need MPI_Recv here?
                MPI_Recv(0, 0, MPI_INT, MASTER, TERMINATION, MPI_COMM_WORLD, &status);
                //printf("termination flag recieved");
                return 0;
            }

            // check pending message with Token tag
            MPI_Iprobe(leftNeighbor, Token, MPI_COMM_WORLD, &tokenFlag, &status);
            if(tokenFlag!=0)
            {
                // need to think about how to pass token
                // http://stackoverflow.com/questions/5972018/

                // initialize receive buffers
                int *unPackedToken = (int*)malloc((n*n+6) * sizeof(int));
                // receive token from previous node
                MPI_Recv(unPackedToken, n*n+6, MPI_INT, leftNeighbor, Token, 
                    MPI_COMM_WORLD, &status);
                // unpack and reconstruct token
                unpackToken(unPackedToken, token);

                //printf("Process %d received token \n", myRank);
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
                    if(tempState)
                        freeState(tempState);
                }
                
                // set global cost to token cost
                global_c = token->c;

                // HEAD node check termination
                if(myRank == MASTER)
                {
                    if( color == WHITE 
                        && token->color == WHITE 
                        && token->count + msgCount == 0)
                    { //send messages with a Termination tag to all other processes and halt
                        //printf("Sending Termination Tag, Stop Master \n");
                        int rank;
                        for (rank = 1; rank < numProcs; rank++) {
                            MPI_Send(0, 0, MPI_INT, rank, TERMINATION, MPI_COMM_WORLD);
                        }
                        printf("******* terminating program ********** \n");
                        return 0;
                    }
                    else
                    {
                        token->color = WHITE;
                        token->count = 0;
                    }
                }
                // SLAVE node update message count
                else
                {
                    if(color == BLACK)
                        token->color = BLACK;
                    token->count = token->count+msgCount;
                }

                // initialize send buffer
                int *packedToken = (int*)malloc((n*n+6) * sizeof(int));
                // pack token
                packToken(token, packedToken);
                // send token to the successor
                MPI_Send(packedToken, n*n+6, MPI_INT, rightNeighbor, Token, MPI_COMM_WORLD);

                color = WHITE;

                // free send and receive buffers
                free(packedToken);
                free(unPackedToken);
            }
            
            // ????? suppose to be while, how to handle?
            // check pending message with Unexamined subproblem tag
            MPI_Iprobe(leftNeighbor, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
            while(unexaminedSubFlag!=0)
            {
                // DEBUG
                printf("node %d receive subproblem \n", myRank);

                // initialize receive buffers
                int *unPackedState = (int*)malloc((n*n+3) * sizeof(int));
                struct state *tempRecvState = (state*)malloc(sizeof(struct state));

                MPI_Recv(unPackedState, n*n+3, MPI_INT, leftNeighbor, UNEXAMINED_SUBPROBLEM, 
                    MPI_COMM_WORLD, &status);
                
                // unpack receive problem and create new state
                unpackState(unPackedState, tempRecvState);

                printBoard(tempRecvState->board, 3);

                msgCount = msgCount - 1;
                color = BLACK;
                if(tempRecvState->lowerBound < global_c)
                    queue.push(*tempRecvState);

                // free receive buffers
                free(unPackedState);
                if(tempRecvState)
                    freeState(tempRecvState);                

                unexaminedSubFlag = 0;
                MPI_Iprobe(leftNeighbor, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
            }

            // if more than one unexamined subproblem in queue, then
            if(queue.size() > 1)
            {
                struct state *tempSendState = (state*)malloc(sizeof(struct state));
                *tempSendState = queue.top();
                queue.pop();

                //DEBUG
                //printf("node %d send subproblem \n", myRank);
                //printf("send lowerBound is %d \n", tempSendState -> lowerBound);

                int *packedState = (int*)malloc((n*n+3) * sizeof(int));
                packState(tempSendState, packedState);
                
                MPI_Send(packedState, n*n+3, MPI_INT, rightNeighbor, UNEXAMINED_SUBPROBLEM, 
                    MPI_COMM_WORLD);

                printBoard(tempSendState->board, 3);

                msgCount = msgCount + 1;
                color = BLACK;

                free(packedState);
                if(tempSendState)
                    freeState(tempSendState);
            }

            /***********************************************************************************/

            gettimeofday(&lastComm, NULL); //get current time
        }
        else if(!queue.empty())
        {
            *currentState = queue.top();
            queue.pop();

            //print board here for debugging
            //printf("Current State \n");
            //printBoard(currentState->board,n);

            //printf("node %d here 9 \n", myRank);

            // ????? what is best_c, is it local_c or global_c
            //int best_c = local_bestState->lowerBound;
            //if(currentState->lowerBound < best_c) //currentState is u in the pseudo code
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
                    int mininumBound = 10000, i, k, chosenDirection;
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
                        nextState = makeAState(k, currentState); //this is v in the pseudo code
                        if(nextState->lowerBound < global_c)
                        {
                            queue.push(*nextState);
                        }
                    }

                    //printf("node %d here 10 \n", myRank);

                    free(directions);
                }
            }
        }
    }
    //printf("freeing local_bestState \n");
    if(local_bestState)
	   freeState(local_bestState);
    //printf("freeing currentState \n");
    if(currentState)
	   freeState(currentState);
    //printf("freeing nextState \n");
	freeState(nextState);
    //printf("freeing initial \n")
	freeState(initial);

	MPI_Finalize();

	return 0;
}

/*	JUNK

	gettimeofday(&currTime, NULL);
	printf("time diff is %d \n", timeDiff(&lastComm, &currTime));

*/
