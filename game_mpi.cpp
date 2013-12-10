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
#define COMM_INTERVAL 20000		/* Time between communication steps */
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
	struct timeval lastComm;			// time of last communication
	int msgCount;						// messages sent minus messages received
	priority_queue<state> queue;		// priority queue, each process has its
                                        // own queue

	// token passed around ring for termination detection
	struct TOKEN *token	= (TOKEN*)malloc(sizeof(struct TOKEN));	

	local_c = INF;
	gettimeofday(&lastComm, NULL);
	msgCount = 0;
	color = WHITE;

	if(myRank == MASTER) //master initialize board
	{
		token->c = INF;
		token->color = WHITE;
		token->count = 0;
		// send token to next process (myRank = 1)
		MPI_Send(token, sizeof(struct TOKEN), MPI_BYTE, 1, Token, 
				MPI_COMM_WORLD);
	}


    //int i = 0;
    //for (int i=0; i<10; i++) //this suppose to repeat forever
    while(true)
    {
            if(queue.empty() || timeDiff(&lastComm) > COMM_INTERVAL)
            {

            /****************************** BandB_Communication() Start ******************************/
                int terminationFlag = 0, tokenFlag = 0, unexaminedSubFlag = 0;
                // get the neighbors in the ring (receive from left, send to right)
                int leftNeighbor = (myRank + numProcs - 1) % numProcs;
                int rightNeighbor = (myRank + 1) % numProcs;

                // check pending message with TERMINATION tag
                // printf("node %d, checking termination tag \n", myRank);
                MPI_Iprobe(MASTER, TERMINATION, MPI_COMM_WORLD, &terminationFlag, &status);
                if(terminationFlag!=0)
                {
                    //clean up receive before finalize
                    ////token receive
                    //MPI_Iprobe(leftNeighbor, Token, MPI_COMM_WORLD, &tokenFlag, &status);
                    //if (tokenFlag!=0){ 
                    //    MPI_Recv(token, sizeof(struct TOKEN), MPI_BYTE, leftNeighbor, Token,
                    //                            MPI_COMM_WORLD, &status);
                    //}

                    //termination receive
                    MPI_Recv(0, 0, MPI_INT, MASTER, TERMINATION, MPI_COMM_WORLD, &status);

                    //Halt the process
                    printf("node %d, termination flag recieved \n", myRank);
                    MPI_Finalize(); 
                }

                // check pending message with Token tag
                // printf("node %d, checking token tag \n", myRank);
                MPI_Iprobe(leftNeighbor, Token, MPI_COMM_WORLD, &tokenFlag, &status);
                if(tokenFlag!=0)
                {
                    MPI_Recv(token, sizeof(struct TOKEN), MPI_BYTE, leftNeighbor, Token, 
                        MPI_COMM_WORLD, &status);

                    //printf("Process %d received token \n", myRank);

                    // HEAD receive token, node check termination
                    if(myRank == MASTER)
                    {
                        printf("*********** Master Received Token Start *************** \n");
                        //if(color == WHITE)
                        //    printf("Master Color White \n");
                        //else
                        //    printf("Master Color Black \n");
                        //

                        printf("msgCount %d \n", msgCount);
                        printToken(token);
                        printf("*********** Master Received Token End *************** \n");

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
                    MPI_Send(token, sizeof(struct TOKEN), MPI_BYTE, rightNeighbor, Token, 
                        MPI_COMM_WORLD);
                    color = WHITE; //after sending message
                }
            /****************************** BandB_Communication() End ******************************/
                gettimeofday(&lastComm, NULL); //get current time
            }


    }

    //MPI_Finalize();
    return 0;
}
