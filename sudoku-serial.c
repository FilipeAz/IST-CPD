#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <limits.h>
#include <ctype.h>


//Global Variables
int L;
int** matrix;
int dimension;
int totalsize;

char** rowvalues; 
char** columnvalues; 
char** sectionvalues;

char*** possibleValues;

void createSudokuValues() {
	for(int i = 0; i < dimension; i++) {
		for (int j = 0; j < dimension; j++) {
			rowvalues[i][j] = 0;
			columnvalues[i][j] = 0;
			sectionvalues[i][j] = 0;
			for (int k = 0; k < dimension; k++) {
				possibleValues[i][j][k] = 0;
			}
		}
	}
}

void printMatrix(int** m) {
	for(int row = 0; row < dimension; row++) {
		for(int column = 0; column < dimension; column++) {
			printf("%d ", m[row][column]);
		}
		printf(" \n");
	}
}



int valueIsNotPossible(int row, int column, int value) {
	//value - 1 because we pass numbers between 1 and L*L
	int v = value - 1;
	int possible = ((rowvalues[row][v]) || (columnvalues[column][v]) || (sectionvalues[(L * (row / L)) + (column / L ) ][v]));
	return possible;
}

void setValue(int row, int column, int value,int flag) {
	int v = value - 1;
	rowvalues[row][v] = flag;
	columnvalues[column][v] = flag;
	sectionvalues[(L * (row / L)) + (column / L ) ][v] = flag;
}



int solveSudoku(int row, int column) {

	if (column == dimension) {
		row++;
		column = 0;
	}

	if (row == dimension) {
		return 1;
	}

	if (matrix[row][column]) {
		if (solveSudoku(row,column+1)) return 1;
		return 0;
	}


	for(int i = 0; i < dimension; i++) {
		if (!possibleValues[row][column][i])
			continue; 
		if (valueIsNotPossible(row, column, i+1))
			continue;
		int number = i+1;
		matrix[row][column] = number;
		setValue(row, column,number, 1); 
		if (solveSudoku(row,column+1)) return 1;
		
		setValue(row,column, number, 0);
		matrix[row][column] = 0;
	}
	return 0;
}



int main(int argc, const char * argv[]) {
	if (argc < 2) {
		printf("Wrong usage: needs input file");
	} else {

		//Open file
		FILE *file;
		file = fopen(argv[1], "r");
	
		//Initialization of L dimension and totalsize			
		char * line = NULL;
    	size_t size = 0;
		getline(&line, &size, file);
		L = strtol(line,&line,10);

		dimension = L*L;
  		totalsize = dimension * dimension;


  		//Space Allocation
  		matrix = (int**) malloc(dimension*sizeof(int*));
		rowvalues = (char**) malloc(dimension*sizeof(char*));
		columnvalues = (char**) malloc(dimension*sizeof(char*));
		sectionvalues = (char**) malloc(dimension*sizeof(char*));
		possibleValues = (char***) malloc(dimension*sizeof(char**));
		for (int i = 0; i < dimension; ++i)
		{	
			matrix[i] = (int*) malloc(dimension*sizeof(int));
			rowvalues[i] = (char*) malloc(dimension*sizeof(char));
			columnvalues[i] = (char*) malloc(dimension*sizeof(char));
			sectionvalues[i] = (char*) malloc(dimension*sizeof(char));
			possibleValues[i] = (char**) malloc(dimension*sizeof(char*));

			for (int j = 0; j < dimension; ++j)
			{
				possibleValues[i][j] = (char*) malloc(dimension*sizeof(char));

			}
		}
  		
		//Set basix values
  		createSudokuValues();


  		//Matrix Initialization
    	for(int i = 0; i < dimension; i++) {
    		char * l = NULL;
    		size_t len = 0;
    		getline(&l, &len, file);
    		char *p = l;
    		for (int j = 0; j < dimension;){ 	
    			if(isdigit(*p)) {
    				int value = strtol(p,&p,10);
    				matrix[i][j] = value;
    				j++;
    			} else {
    				p++;
    			}
    		}	
    	}
    	fclose(file);

	    //SetValues
  		for(int i = 0; i < dimension; i++) {
  			for(int j = 0; j < dimension; j++) {
  				if (matrix[i][j] != 0) 
  					setValue(i,j,matrix[i][j], 1);
  			}
  		}

  		//Set values not possible
  		for(int i = 0; i < dimension; i++) {
  			for(int j = 0; j < dimension; j++) {

  				for(int value = 1; value < dimension + 1; value++) {
					if(!valueIsNotPossible(i, j, value)) {
						possibleValues[i][j][value-1] = 1;
					}
				}
  			}
  		}

  		if (solveSudoku(0,0))
  			printMatrix(matrix);
  		else {
  			printf("No solution");
  		}

  		for (int i = 0; i < dimension; i++) {
			free(rowvalues[i]);
			free(columnvalues[i]);
			free(sectionvalues[i]);
			free(matrix[i]);
			for(int j = 0; j < dimension; j++) {
				free(possibleValues[i][j]);
			}
			free(possibleValues[i]);
		}
		free(rowvalues);
		free(columnvalues);
		free(sectionvalues);
		free(matrix);
		free(possibleValues);

  	}

	return 0; 
}
