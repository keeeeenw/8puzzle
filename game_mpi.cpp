/*
 *
 * Author: Yu Zhao and Zixiao Wang
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
#define COMM_INTERVAL 0.0001		/* Time between communication steps */
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
	int myRank, numProcs;				// MPI related variables

	// initialize MPI
    MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
	MPI_Status status;

    // initialize variables
	enum Color color;					// process color (for termination detection)
	int global_c, local_c;				// cost of global and local best solution so far
	//struct timeval lastComm;			// time of last communication
	double timeval, last_comm;			// time of last communication
	int msgCount;						// messages sent minus messages received
	priority_queue<state> queue;		// priority queue, each process has its
                                        // own queue

	// token passed around ring for termination detection
	struct TOKEN *token	= (TOKEN*)malloc(sizeof(struct TOKEN));	

    // initialize some local variables
	local_c = INF;
	//gettimeofday(&lastComm, NULL);
    last_comm = MPI_Wtime();
    //printf("last_comm is %f \n", last_comm);
	msgCount = 0;
	color = WHITE;

    // initial state of the board
    struct state *initial = (state*)malloc(sizeof(struct state));
	int n = 3; //board size is 3x3

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

	if(myRank == MASTER) //master initialize board
	{
        //create board
		int *board;
		board = (int*)malloc(n*n * sizeof(int));
		// fill specific board 
		board[0] = 8;
		board[1] = 7;
		board[2] = 0;
		board[3] = 2;
		board[4] = 3;
		board[5] = 6;
		board[6] = 4;
		board[7] = 5;
		board[8] = 1;

		// check solvability
		while(!isSolvable(board, n)){
			shuffleBoard(board, n*n);
		}

        // set initial set
		setState(initial, board, n, 0);
        printf("Initial \n");
        printBoard(initial->board, n);

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
        MPI_Ssend(initialPackedToken, n*n+6, MPI_INT, 1, Token, MPI_COMM_WORLD);
        // free send buffer
        free(initialPackedToken);
        color = WHITE;
	}


    int ic = 0;
    for (int ic=0; ic<1000; ic++) //this suppose to repeat forever
    //while(true)
    {
        //if(queue.empty() || timeDiff(&lastComm) > COMM_INTERVAL)
        if(queue.empty() || MPI_Wtime()-last_comm > COMM_INTERVAL)
        {

            /****************************** BandB_Communication() Start ******************************/
            int terminationFlag = 0, tokenFlag = 0, unexaminedSubFlag = 0;
            // get the neighbors in the ring (receive from left, send to right)
            int leftNeighbor = (myRank + numProcs - 1) % numProcs;
            int rightNeighbor = (myRank + 1) % numProcs;

            /*********************** handle TERMINATION tag ****************/
            // check pending message with TERMINATION tag
            // printf("node %d, checking termination tag \n", myRank);
            MPI_Iprobe(MASTER, TERMINATION, MPI_COMM_WORLD, &terminationFlag, &status);
            if(terminationFlag!=0)
            {
                //termination receive
                MPI_Recv(0, 0, MPI_INT, MASTER, TERMINATION, MPI_COMM_WORLD, &status);

                //Halt the process
                printf("node %d, termination flag recieved \n", myRank);
                MPI_Finalize(); 
                return 0;
            }

            /*********************** handle Token tag ****************/
            // check pending message with Token tag
            // printf("node %d, checking token tag \n", myRank);
            MPI_Iprobe(leftNeighbor, Token, MPI_COMM_WORLD, &tokenFlag, &status);
            if(tokenFlag!=0)
            {
                //MPI_Recv(token, sizeof(struct TOKEN), MPI_BYTE, leftNeighbor, Token, 
                //    MPI_COMM_WORLD, &status);

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

                // HEAD receive token, node check termination
                if(myRank == MASTER)
                {
                    //printf("*********** Master Received Token Start *************** \n");
                    //printf("msgCount %d \n", msgCount);
                    //printToken(token);
                    //printf("*********** Master Received Token End *************** \n");

                    if( color == WHITE 
                        && token->color == WHITE 
                        && token->count + msgCount == 0)
                    { 
                        printf("Sending Termination Tag, Stop Master \n");
                        int rank;
                        for (rank = 1; rank < numProcs; rank++) {
                            MPI_Send(0, 0, MPI_INT, rank, TERMINATION, MPI_COMM_WORLD);
                        }
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

                //printToken(token);
                //MPI_Ssend(token, sizeof(struct TOKEN), MPI_BYTE, rightNeighbor, Token, 
                //    MPI_COMM_WORLD);

                //printToken(token);

                //initialize send buffer
                int *packedToken = (int*)malloc((n*n+6) * sizeof(int));
                // pack token
                packToken(token, packedToken);
                // send token to the successor
                MPI_Ssend(packedToken, n*n+6, MPI_INT, rightNeighbor, Token, MPI_COMM_WORLD);
                color = WHITE; //after sending message
            }

            /*********************** handle Unexamined_Subproblem tag ****************/
            //printf("node %d probing unexamined subproblem \n", myRank);
            MPI_Iprobe(leftNeighbor, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
            while(unexaminedSubFlag!=0)
            {
                printf("node %d received unexamined subproblem \n", myRank);

                // initialize receive buffers
                int *unPackedState = (int*)malloc((n*n+3) * sizeof(int));
                struct state *tempRecvState = (state*)malloc(sizeof(struct state));

                MPI_Recv(unPackedState, n*n+3, MPI_INT, leftNeighbor, UNEXAMINED_SUBPROBLEM, 
                    MPI_COMM_WORLD, &status);

                //MPI_Recv(&tempRecvState, sizeof(struct state), MPI_BYTE, leftNeighbor, 
                //    UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &status);
                
                // unpack receive problem and create new state
                unpackState(unPackedState, tempRecvState);
                printState(tempRecvState);
                free(unPackedState);

                msgCount = msgCount - 1;
                color = BLACK;
                if(tempRecvState->lowerBound < global_c)
                {
                    queue.push(*tempRecvState);
                }

                //printf("freeing tempRecvState \n");
                //freeState(tempRecvState);
                unexaminedSubFlag = 0;
                MPI_Iprobe(leftNeighbor, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
            }

            /******************** handle subproblem in q **************************/
            // if more than one unexamined subproblem in queue, then
            if(queue.size() > 1) //change this back to 1
            {
                printf("node %d has unexamined subproblem \n", myRank);
                struct state *tempSendState = (state*)malloc(sizeof(struct state));
                *tempSendState = queue.top();
                queue.pop();

                int *packedState = (int*)malloc((n*n+3) * sizeof(int));
                packState(tempSendState, packedState);
                printState(tempSendState);
                
                MPI_Ssend(packedState, n*n+3, MPI_INT, rightNeighbor, UNEXAMINED_SUBPROBLEM, 
                    MPI_COMM_WORLD);

                //MPI_Send(&tempSendState, sizeof(struct state), MPI_BYTE, rightNeighbor, 
                //    UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD);

                free(packedState);

                msgCount = msgCount + 1;
                color = BLACK;
            }

            /****************************** BandB_Communication() End ******************************/
           //gettimeofday(&lastComm, NULL); //get current time
           last_comm = MPI_Wtime();
        }
        else if(!queue.empty()) //distribute the work
        {
            *currentState = queue.top();
            queue.pop();
            printf("queue not empty on node %d, current state: \n", myRank);
            printState(currentState);

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
                        nextState = makeAState(k, currentState); //this is v in the pseudo code
                        if(nextState->lowerBound < global_c)
                        {
                            printf("node %d, next state: \n", myRank);
                            printState(nextState);
                            queue.push(*nextState);
                        }
                    }
                    //printPQueue(queue);

                    ////printf("node %d here 10 \n", myRank);

                    free(directions);
                }
            }



        }


    }
}
