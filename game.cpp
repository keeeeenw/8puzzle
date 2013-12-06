#include <stdio.h>		// for printf
#include <stdlib.h>		// for malloc
#include <string.h>		// for memcpy
#include <iostream>		// for CPP
#include <queue>		// for priority queue
using namespace std;

struct state
{
	//Call(int *board, int dim, int moveSoFar, int lowerBound) :
    //CallBoard(board), CallDim(dim), CallLowerBound(lowerBound) {}

	int *board;
	int dim;
	int moveSoFar;
	int lowerBound;

    //use friend so we can compare the two states
	friend bool operator<(const state& lhs, const state& rhs)
	{
		return lhs.lowerBound > rhs.lowerBound;
	}
};

//struct Comp{
//	friend bool operator<(const state& lhs, const state& rhs)
//	{
//		return lhs.lowerBound > rhs.lowerBound;
//	}
//};


void shuffleBoard(int *array, int n)
{
	if (n > 1){ 
		int i;
		for (i = 0; i < n - 1; i++){
			int j = i + rand() / (RAND_MAX / (n - i) + 1);
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

void fillBoard(int *board, int dim)
{
	int i;
	for(i=0; i<(dim*dim); i++){
		board[i] = i;
	} 
}

void printBoard(int *board, int dim)
{
	int i, j;
	for(i=0; i<dim; i++){
		for(j=0; j<dim; j++){
			printf("%d\t", board[dim*i+j]);
		}
		printf("\n");
	}
}

void printState(struct state *move)
{
    printf("lowerBound: %d \n", move->lowerBound);
    printBoard(move->board, move->dim);
}


int getManhattan(int i, int dim, int sourceRow, int sourceCol)
{
	int targetRow = (i-1) / dim;
	int targetCol = (i-1) % dim;
	return abs(sourceRow - targetRow)+abs(sourceCol - targetCol);
}

int getBoardManhattan(int *board, int dim)
{
	int i, j, sum=0, temp, item;
	for(i=0; i<dim; i++){
		for(j=0; j<dim; j++){
            item = board[dim*i+j];
            if(item != 0){
                temp = getManhattan(item, dim, i, j);
			    sum = sum + temp;
            }
		}
	}
	return sum;
}

bool checkResult(int *board, int dim)
{
	int i, j;
    for(i=0; i<dim*dim; i++){
        //solution[i] = i+1;
        if(i == dim*dim-1){
            if(board[i]!=0)
                return false;
        } else {
            if(board[i] != i+1)
                return false;
        }
    }

    return true;
    
	//bool correct = true;
	//for(i=0; i<dim; i++){
	//	for(j=0; j<dim; j++){
	//		//if(i != dim-1 && j != dim-1){
	//		//	if(board[dim*i+j] != dim*i+j+1){
	//		//		//correct = false;
    //        //        return false;
	//		//	}
	//		//}
    //        //if(board[dim*i+j] != dim*(i+1)+j+1){
    //        //    return false;
    //        //}
	//	}
	//}
	//return correct;
}

void setState(state *newMove, int *board, int dim, int moveSoFar)
{
	newMove->board = board;
	newMove->dim = dim;
	newMove->moveSoFar = moveSoFar;
	newMove->lowerBound = moveSoFar + getBoardManhattan(board, dim);
}

void freeState(state *Move)
{
	free(Move->board);
	free(Move);
}

int* moveHole(int direction, int *board, int dim)
{
	int holeRow, holeCol, temp, i;
	// create a new board same as the old one
	int *newBoard;
	size_t size = dim * dim * sizeof(int);
	newBoard = (int*)malloc(size);
	memcpy(newBoard, board, size);
	// Find out the location of the hole
	for(i=0; i<dim*dim; i++){
		if(newBoard[i]==0){
			holeRow = i / dim;
			holeCol = i % dim;
		}
	}
	// make the move
	if(direction == 0)
	{
		// 0 is move up
		temp = newBoard[(holeRow-1)*dim+holeCol];
		newBoard[(holeRow-1)*dim+holeCol] = 0;
		newBoard[holeRow*dim+holeCol] = temp;
	}
	else if(direction == 1)
	{
		// 1 is move down
		temp = newBoard[(holeRow+1)*dim+holeCol];
		newBoard[(holeRow+1)*dim+holeCol] = 0;
		newBoard[holeRow*dim+holeCol] = temp;
	}
	else if(direction == 2)
	{
		// 2 is move left
		temp = newBoard[holeRow*dim+(holeCol-1)];
		newBoard[holeRow*dim+(holeCol-1)] = 0;
		newBoard[holeRow*dim+holeCol] = temp;
	}
	else if(direction == 3)
	{
		// 3 is move right
		temp = newBoard[holeRow*dim+(holeCol+1)];
		newBoard[holeRow*dim+(holeCol+1)] = 0;
		newBoard[holeRow*dim+holeCol] = temp;
	}
	else
	{
		printf("Error moving at direction %d \n", direction);
	}

    //printf("New Board: \n");
    //printBoard(newBoard, dim);

	return newBoard;
}

state* makeAMove(int direction, state *currentMove)
{
	struct state *nextMove = (state*)malloc(sizeof(struct state));
	setState(nextMove, moveHole(direction, currentMove->board, currentMove->dim), 
		currentMove->dim, currentMove->moveSoFar + 1);
	return nextMove;
}

int setHoleDirection(int *directions, int holeRow, int holeCol, int n)
{
    /* 0 is up, 1 is down, 2 is left, 3 is right*/
    int numDirections;

    if(holeRow == 0){
        if(holeCol == 0){ //upper left
            numDirections = 2;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 1;
            directions[1] = 3;
        }else if(holeCol == n-1){ //upper right
            numDirections = 2;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 1;
            directions[1] = 2;
        }else{ // upper middle
            numDirections = 3;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 1;
            directions[1] = 2;
            directions[2] = 3;
        }
    }else if(holeRow == n-1){
        if(holeCol == 0){ //buttom left
            numDirections = 2;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 3;
        }else if(holeCol == n-1){ //buttom right
            numDirections = 2;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 2;
        }else{ //buttom middle
            numDirections = 3;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 2;
            directions[2] = 3;
        }
    }else{
        if(holeCol == 0){ //middle left
            numDirections = 3;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 1;
            directions[2] = 3;
        }else if(holeCol == n-1){ //middle right
            numDirections = 3;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 1;
            directions[2] = 2;
        }else{ //middle middle
            numDirections = 4;
            // directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 1;
            directions[2] = 2;
            directions[3] = 3;
        }
    }
    return numDirections;
}

void printPQueue(priority_queue<state> queue)
{
	priority_queue<state> queueCopy;
    printf("------Prority Queue Start-----------\n");
	struct state *currentState  = (state*)malloc(sizeof(struct state));
    while (!queue.empty())
    {
        *currentState = queue.top();
        queue.pop();
        printState(currentState);
        queueCopy.push(*currentState); //put it to copy after print
    }
    printf("------Prority Queue End-----------\n");

    queue = queueCopy;
}

bool compareBoard(int *board1, int *board2, int dim)
{
    int i;
    for(i=0;i<dim*dim;i++){
        if(board1[i] != board2[i])
            return false;
    }
    return true;
}

bool pqueueContain(priority_queue<state> queue, struct state *newState, int dim)
{
    bool flag = false; //true it the queue contains newState
	priority_queue<state> queueCopy;
	struct state *currentState  = (state*)malloc(sizeof(struct state));
    while (!queue.empty())
    {
        *currentState = queue.top();
        queue.pop();
        //compare currentState with newState
        if(flag) //we do not want to reset it if it is true already
            flag = compareBoard(currentState->board, newState->board, dim);
        queueCopy.push(*currentState); //put it to copy after print
        
    }

    queue = queueCopy;

    return flag;
}

bool isSolvable(int *board, int dim)
{
	int i, j, zeroPos, blankRow, inversions;
	int size = dim*dim;
	bool solvable = false;

	// Create a copy of the original board
	int *newBoard;
	newBoard = (int*)malloc(sizeof(size));
	memcpy(newBoard, board, size);
	
	// get the position of the zero element
	for(i=0; i<size; i++){
		if(newBoard[i]==0){
			zeroPos = i;
		}
	}

	// get the row where zero is located
	blankRow = zeroPos/dim;

	// delete zero element
	for (i=zeroPos; i<size-1; i++){
		newBoard[i] = newBoard[i+1];
	}

	// get num of inversions of the newBoard
	for(i=0; i<size-2; i++){
		for(j=i+1; j<size-1; j++){
			if(newBoard[i] > newBoard[j]){
				inversions++;
			} 
		}
	}

	// check base on inversion rules
	if(dim % 2 == 0){
		if((blankRow+inversions) % 2 != 0){
			solvable = true;
		}
	}else{
		if(inversions % 2 == 0){
			solvable = true;
		}
	}

	free(newBoard);

	return solvable;
}

int main(int argc, char *argv[])
{
	int n = 3;
	int *board;
	int *bestSolution;
	struct state *currentMove  = (state*)malloc(sizeof(struct state));

	//priority_queue<state, vector<state>, Comp> queue;
	priority_queue<state> queue;

    //building board
	srand(time(NULL));
	board = (int*)malloc(n*n * sizeof(int));
	fillBoard(board, n);
	shuffleBoard(board, n*n);

	//board[0] = 1;
	//board[1] = 5;
	//board[2] = 2;
	//board[3] = 4;
	//board[4] = 3;
	//board[5] = 0;
	//board[6] = 7;
	//board[7] = 8;
	//board[8] = 6;

	while(!isSolvable(board, n)){
		shuffleBoard(board, n*n);
	}

	// DEBUG
	// printf("Manhattan Distance is %d \n", getBoardManhattan(board, n));

	struct state *initial = (state*)malloc(sizeof(struct state));
	setState(initial, board, n, 0);

	// DEBUG
    printf("Problem Starts. Here is our board: \n");
	printBoard(initial->board, n);
	printf("lowerBound is %d \n", initial->lowerBound);

	queue.push(*initial);

	////while (!queue.empty())

	int j;
	//for(j=0; j<100000; j++)
    while (!queue.empty())
	{
        //printPQueue(queue);
		*currentMove = queue.top();
		struct state *nextMove;
		queue.pop();

        // DEBUG - check the lowerbound
        //printf("Checking currentMove \n");
        //printState(currentMove);

		if(checkResult(currentMove->board, n)) //finished
		{
			printBoard(currentMove->board, n);
			exit(0);
            //break;
		}
		else
		{
			int mininumBound = 10000, i, k, chosenDirection;
			int holeCol, holeRow;

			// Find out the location of the hole
			for(i=0; i<n*n; i++){
				if(currentMove->board[i]==0){
					holeRow = i / n;
					holeCol = i % n;
				}
			}

            // Find directions of the moves based on the location of the hole
            int numDirections = 4; //at most 4 directions
            int *directions = (int*)malloc(numDirections * sizeof(int));
            numDirections = setHoleDirection(directions, holeRow, holeCol, n);

            // DEBUG
            j++;
            if(j % 100 == 0){
            	printf("++++++++++ iteration %d ++++++++++++\n", j);
            	printf("Current Board: \n");
            	printf("lowerBound is %d and moveSoFar is %d \n", currentMove->lowerBound, currentMove->moveSoFar);
				printBoard(currentMove->board, n);
                std::cout << "pq size: " << queue.size() << '\n';
            }

            // For each direction, find the nextMove
			for(i=0; i<numDirections; i++)
			{
				k = directions[i];
                //printf("Direction %d\n", k);
				struct state *nextMove;
				nextMove = makeAMove(k, currentMove);
				//printf("lowerBound is %d and moveSoFar is %d \n", nextMove->lowerBound, nextMove->moveSoFar);
				//printBoard(nextMove->board, n);		
				// DEBUG
                //printf("Info on next possible move \n");
				//printBoard(nextMove->board, n);
				//printf("move direction %d, lowerboud %d \n", k, nextMove->lowerBound);
				//printf("move direction %d, Manhattan total %d \n", k, getBoardManhattan(nextMove->board, n));

                // we need to make sure that we do not add duplicate?
                // we need to keep track of the board we worked on 
                //if(!pqueueContain(queue, nextMove, n))
				queue.push(*nextMove);
			}
            free(directions);
		}
	}
    freeState(currentMove);
    freeState(initial);

	//system("pause");

	return 0;
}

// junk

//std::cout << queue.top() << std::endl;
			//	if(possibleNextMove->lowerBound < mininumBound){
			//		nextMove = possibleNextMove;
			//		mininumBound = possibleNextMove->lowerBound;
			//		chosenDirection = k;
			//		//printf("Manhattan Distance is %d \n", getBoardManhattan(nextMove->board, n));
			//	}else{
			//		freeMove(possibleNextMove);
			//	}

			//	printf("lowerBound Finalized is %d \n", nextMove->lowerBound);
			//	printf("chosenDirection is %d \n", chosenDirection);
			//	printBoard(nextMove->board, n);			
