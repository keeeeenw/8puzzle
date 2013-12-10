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

enum Color{ WHITE, BLACK };

struct TOKEN
{
	int c;
	enum Color color;
	int count;
	struct state s;
};

void printToken(struct TOKEN *token)
{
    printf("-------- Token Start ------------\n");
    printf("cost %d \n", token->c);
    printf("count %d \n", token->count);
    if(token->color == WHITE)
        printf("Token Color White \n");
    else
        printf("Token Color Black \n");
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
        //printf("From master, size of token struct: %lu \n", sizeof(struct TOKEN));
        //printf("From master, size of state struct: %lu \n", sizeof(struct state));
        //printf("From master, size of int: %lu \n", sizeof(int));
		MPI_Ssend(token, sizeof(struct TOKEN), MPI_BYTE, 1, Token, 
				MPI_COMM_WORLD);
        color = WHITE;
	}


    //int i = 0;
    //for (int i=0; i<1000; i++) //this suppose to repeat forever
    while(true)
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
                MPI_Recv(token, sizeof(struct TOKEN), MPI_BYTE, leftNeighbor, Token, 
                    MPI_COMM_WORLD, &status);
                if(myRank != MASTER)
                    msgCount--;

                //printf("Process %d received token \n", myRank);

                // HEAD receive token, node check termination
                if(myRank == MASTER)
                {
                    //printf("*********** Master Received Token Start *************** \n");
                    ////if(color == WHITE)
                    ////    printf("Master Color White \n");
                    ////else
                    ////    printf("Master Color Black \n");
                    ////

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
                else //SLAVE node update message count
                {
                    if(color == BLACK)
                        token->color = BLACK;
                    token->count = token->count+msgCount;
                }


                // forward token to the successor
                msgCount++;
                color = BLACK;
                //printToken(token);
                MPI_Ssend(token, sizeof(struct TOKEN), MPI_BYTE, rightNeighbor, Token, 
                    MPI_COMM_WORLD);
                color = WHITE; //after sending message
            }

            /*********************** handle Unexamined_Subproblem tag ****************/
            //printf("node %d probing unexamined subproblem \n", myRank);
            MPI_Iprobe(leftNeighbor, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
            while(unexaminedSubFlag!=0)
            {
                printf("node %d received unexamined subproblem \n", myRank);
	            struct state *tempRecvState  = (state*)malloc(sizeof(struct state));
                MPI_Recv(&tempRecvState, sizeof(struct state), MPI_BYTE, leftNeighbor, 
                    UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &status);
                //printState(tempRecvState);
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
            if(queue.size() > 0) //change this back to 1
            {
                printf("node %d has unexamined subproblem \n", myRank);
                struct state *tempSendState = (state*)malloc(sizeof(struct state));
                *tempSendState = queue.top();
                queue.pop();
                //printState(tempSendState);
                MPI_Send(&tempSendState, sizeof(struct state), MPI_BYTE, rightNeighbor, 
                    UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD);
                msgCount = msgCount + 1;
                color = BLACK;
            }

            /****************************** BandB_Communication() End ******************************/
           //gettimeofday(&lastComm, NULL); //get current time
           last_comm = MPI_Wtime();
        }
        else if(!queue.empty()) //distribute the work
        {
            printf("queue not empty on node %d \n", myRank);
        }


    }

    //MPI_Finalize();
    return 0;
}
