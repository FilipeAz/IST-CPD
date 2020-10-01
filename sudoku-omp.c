#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <omp.h>

//Global Vatiables 
int L;
int dimension;
int totalsize;


char*** possibleValues;

int flag_threads;



typedef struct states{
	char** rowvalues; 
	char** columnvalues; 
	char** sectionvalues;
	int** matrix;
} state;


void createSudokuValues(state* s) {
	for(int i = 0; i < dimension; i++) {
		for (int j = 0; j < dimension; j++) {
			s->rowvalues[i][j] = 0;
			s->columnvalues[i][j] = 0;
			s->sectionvalues[i][j] = 0;
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

void copyStates(state* s_in, state* s_out) {
	s_out->matrix = (int**) malloc(dimension*sizeof(int*));
	s_out->rowvalues = (char**) malloc(dimension*sizeof(char*));
	s_out->columnvalues = (char**) malloc(dimension*sizeof(char*));
	s_out->sectionvalues = (char**) malloc(dimension*sizeof(char*));
	for (int i = 0; i < dimension; i++) {
		s_out->matrix[i] = (int*) malloc(dimension*sizeof(int));
		s_out->rowvalues[i] = (char*) malloc(dimension*sizeof(char));
		s_out->columnvalues[i] = (char*) malloc(dimension*sizeof(char));
		s_out->sectionvalues[i] = (char*) malloc(dimension*sizeof(char));
		//Copy all values from stateIn to stateOut
		memcpy(s_out->matrix[i], s_in->matrix[i], dimension * sizeof(int));
		memcpy(s_out->rowvalues[i], s_in->rowvalues[i], dimension * sizeof(char));
		memcpy(s_out->columnvalues[i], s_in->columnvalues[i], dimension * sizeof(char));
		memcpy(s_out->sectionvalues[i], s_in->sectionvalues[i], dimension * sizeof(char));
	}
}

void deleteState(state* s) {
	for (int i = 0; i < dimension; i++) {
		free(s->rowvalues[i]);
		free(s->columnvalues[i]);
		free(s->sectionvalues[i]);
		free(s->matrix[i]);
	}
	free(s->rowvalues);
	free(s->columnvalues);
	free(s->sectionvalues);
	free(s->matrix);
}

int valueIsNotPossible(state* s, int row, int column, int value) {
	//value - 1 because we pass numbers between 1 and L*L
	int v = value - 1;
	int possible = ((s->rowvalues[row][v]) || (s->columnvalues[column][v]) || (s->sectionvalues[(L * (row / L)) + (column / L ) ][v]));
	return possible;
}

void setValue(state* s, int row, int column, int value,int flag) {
	int v = value - 1;
	s->rowvalues[row][v] = flag;
	s->columnvalues[column][v] = flag;
	s->sectionvalues[(L * (row / L)) + (column / L ) ][v] = flag;
}


int solveSudoku(state *s ,int row, int column) {
	if (column == dimension) {
		row++;
		column = 0;
	}

	if (row == dimension) {
		#pragma omp critical 
		{
			printMatrix(s->matrix);
			exit(1);
		}
	}

	if (s->matrix[row][column]) {
		return solveSudoku(s, row,column+1);
		
	}

	for(int i = 0; i < dimension; i++) {

		if (!possibleValues[row][column][i])
			continue; 
		if (valueIsNotPossible(s, row, column, i+1))
			continue;

		int number = i+1;
		s->matrix[row][column] = number;
		setValue(s,row, column,number, 1);


		if (flag_threads > 0){
			state s2;
			copyStates(s, &s2);

			#pragma omp atomic
			flag_threads--;

			#pragma omp task
			{
				solveSudoku(&s2, row, column+1);
				deleteState(&s2);
				#pragma omp atomic
				flag_threads++;
			}

		} else{
			solveSudoku(s, row,column+1);
		}

		setValue(s,row,column, number, 0);
		s->matrix[row][column] = 0;
	}
	return 0;
}



int main(int argc, const char * argv[]) {
	if (argc < 2) {
		int x = 0;
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


  		//Initial State initialization

  		state initialstate;


  		//Space Allocation
  		initialstate.matrix = (int**) malloc(dimension*sizeof(int*));
		initialstate.rowvalues = (char**) malloc(dimension*sizeof(char*));
		initialstate.columnvalues = (char**) malloc(dimension*sizeof(char*));
		initialstate.sectionvalues = (char**) malloc(dimension*sizeof(char*));
		possibleValues = (char***) malloc(dimension*sizeof(char**));
		for (int i = 0; i < dimension; ++i)
		{	
			initialstate.matrix[i] = (int*) malloc(dimension*sizeof(int));
			initialstate.rowvalues[i] = (char*) malloc(dimension*sizeof(char));
			initialstate.columnvalues[i] = (char*) malloc(dimension*sizeof(char));
			initialstate.sectionvalues[i] = (char*) malloc(dimension*sizeof(char));
			possibleValues[i] = (char**) malloc(dimension*sizeof(char*));

			for (int j = 0; j < dimension; ++j)
			{
				possibleValues[i][j] = (char*) malloc(dimension*sizeof(char));

			}
		}
  		
		//Set basix values
  		createSudokuValues(&initialstate);


  		//Matrix Initialization
    	for(int i = 0; i < dimension; i++) {
    		char * l = NULL;
    		size_t len = 0;
    		getline(&l, &len, file);
    		char *p = l;
    		for (int j = 0; j < dimension;){ 	
    			if(isdigit(*p)) {
    				int value = strtol(p,&p,10);
    				initialstate.matrix[i][j] = value;
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
  				if (initialstate.matrix[i][j] != 0) 
  					setValue(&initialstate, i,j,initialstate.matrix[i][j], 1);
  			}
  		}

  		//Set values not possible
  		for(int i = 0; i < dimension; i++) {
  			for(int j = 0; j < dimension; j++) {

  				for(int value = 1; value < dimension + 1; value++) {
					if(!valueIsNotPossible(&initialstate, i, j, value)) {
						possibleValues[i][j][value-1] = 1;
					}
				}
  			}
  		}

  		//Get number of threads
  		flag_threads = omp_get_max_threads() - 1;

  		#pragma omp parallel  
  			#pragma omp single 
			{	
				solveSudoku(&initialstate,0,0);
				#pragma omp atomic
				flag_threads++;
			}

  		printf("No solution");

  	}

	return 0; 
}
