#include <stdio.h>
#include <stdlib.h>
<<<<<<< HEAD
#include <iostream>
#include <queue>

struct move{
	int *board;
	int dim;
	int moveSoFar;
	int lowerBound;
};
=======
>>>>>>> 2c38e01c75fffb07f75a5bd96f8f9208f1f59a80

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

int getManhattan(int i, int dim, int sourceRow, int sourceCol)
{
	int targetRow = (i-1) / dim;
	int targetCol = (i-1) % dim;
	return abs(sourceRow - targetRow)+abs(sourceCol - targetCol);
}

int getBoardManhattan(int *board, int dim)
{
	int i, j, sum, temp;
	for(i=0; i<dim; i++){
		for(j=0; j<dim; j++){
			temp = getManhattan(board[dim*i+j], dim, i, j);
			sum = sum + temp;
		}
	}
	return sum;
}

<<<<<<< HEAD
bool checkResult(int *board, int dim)
{
	int i, j;
	bool correct = true;
	for(i=0; i<dim; i++){
		for(j=0; j<dim; j++){
			if(i != dim-1 && j != dim-1){
				if(board[dim*i+j] != dim*i+j+1){
					correct = false;
				}
			}
		}
	}
	return correct;
}

void setMove(move *newMove, int *board, int dim, int moveSoFar){
	newMove->board = board;
	newMove->dim = dim;
	newMove->moveSoFar = moveSoFar;
	newMove->lowerBound = moveSoFar + getBoardManhattan(board, dim);
}

int* makeAMove(int direction, move *currentMove){
	if(direction == 0){
		// 0 is move up
	}else if(direction == 1){
		// 1 is move down
	}else if(direction == 2){
		// 2 is move left
	}else if(direction == 3){
		// 3 is move right
	}else{}
}

=======
>>>>>>> 2c38e01c75fffb07f75a5bd96f8f9208f1f59a80
int main(int argc, char *argv[])
{
	int n = 4;
	int *board;
<<<<<<< HEAD
	int *currentBoard;
	int *bestSolution;

	board = (int*)malloc(n*n * sizeof(int));
	fillBoard(board, n);
	shuffleBoard(board, n*n);
	//printBoard(board, n);
	//printf("Manhattan Distance is %d \n", getBoardManhattan(board, n));

	struct move *initial = (move*)malloc(sizeof(struct move));
	setMove(initial, board, n, 0);
	//printBoard(initial->board, initial->dim);

	std::priority_queue<int*> queue;

	queue.push(initial);

	while (!queue.empty())
	{
		currentMove = queue.top();
		queue.pop();
		if(checkResult(currentMove->board, n))
		{
			printBoard(currentMove->board, n);
			exit(0);
		}else
		{

		}

	}

	system("pause");
=======
	board = (int*)malloc(n*n * sizeof(int));
	fillBoard(board, n);
	shuffleBoard(board, n*n);
	printBoard(board, n);
	printf("Manhattan Distance is %d \n", getBoardManhattan(board, n));
>>>>>>> 2c38e01c75fffb07f75a5bd96f8f9208f1f59a80

	return 0;
}

<<<<<<< HEAD
// junk

//std::cout << queue.top() << std::endl;
=======
>>>>>>> 2c38e01c75fffb07f75a5bd96f8f9208f1f59a80
