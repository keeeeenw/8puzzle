#include <stdio.h>
#include <stdlib.h>

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

int main(void)
{
	int n = 4;
	int *board;
	board = (int*)malloc(n*n * sizeof(int));
	fillBoard(board, n);
	shuffleBoard(board, n*n);
	printBoard(board, n);
	printf("Manhattan Distance is %d \n", getBoardManhattan(board, n));
}

