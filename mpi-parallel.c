#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include <omp.h>
#include <mpi.h>

//Global Vatiables 
int L;
int dimension;
int totalsize;

int sendWork = 0;


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

void printValues(char** m) {
	for(int row = 0; row < dimension; row++) {
		for(int column = 0; column < dimension; column++) {
			printf("%d ", m[row][column]);
		}
		printf(" \n");
	}
}

void printPossibleValues() {
	for(int i = 0; i < dimension; i++) {
		printf("row %d ", i);
		for(int j = 0; j < dimension; j++) {
			printf("column %d :", j);
			for (int k = 0; k < dimension; k++)
				printf("%d ", possibleValues[i][j][k]);
		}
		printf("\n");
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

void sendMatrix(int** m, int target){
	for(int i = 0; i < dimension; i++) {
        MPI_Send(m[i], dimension, MPI_INT, target, 2, MPI_COMM_WORLD); 
    }
}

void sendValues(char** m,int target, int rcs) {
	for(int i = 0; i < dimension; i++) {
        MPI_Send(m[i], dimension, MPI_CHAR, target, 2+rcs, MPI_COMM_WORLD); 
    }
}

void recvMatrix(int target, int ** rcv_matrix) {
	for (int i = 0; i < dimension; ++i){	
		MPI_Recv(rcv_matrix[i], dimension, MPI_INT, target, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
}

//rcs : tag corresponding to the values we will receive
// 1 to rowValues, 2 to columnValues and 3 to sectionValues
void recvValues(int target, int rcs, char ** rcv_matrix) { 
	MPI_Status st;
	for (int i = 0; i < dimension; i++){	
		MPI_Recv(rcv_matrix[i], dimension, MPI_CHAR, target, 2+rcs, MPI_COMM_WORLD, &st);
		
	}
}


void elimination(state* s, int row, int column, int* eliminationValues) {
	//First Value is the dimension
	eliminationValues[0] = 0;

	for(int i = 0; i < dimension; i++) {
		if (!possibleValues[row][column][i])
			continue; 
		if (valueIsNotPossible(s, row, column, i+1))
			continue;
		eliminationValues[0] = eliminationValues[0] + 1;
		eliminationValues[eliminationValues[0]] = i+1;
	}
}

int solveSudoku(state *s ,int row, int column) {
	int me, nprocs;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
	

	if (column == dimension) {
		row++;
		column = 0;
	}

	if (row == dimension) {
		int finalWork = -1;
		int recvWork;
		if(nprocs != 1) {
			MPI_Send(&finalWork, 1, MPI_INT, (me + 1) % nprocs, 1, MPI_COMM_WORLD);
			if (me != 0) sendMatrix(s->matrix, 0);
		} 
		return 1;
	}

	if (s->matrix[row][column]) {
		return solveSudoku(s, row,column+1);	
	}

	int * eliminationValues = (int*) malloc((dimension + 1)*sizeof(int));
	elimination(s, row, column, eliminationValues);

	if (eliminationValues[0] == 1) {
		int number = eliminationValues[1];
		s->matrix[row][column] = number;
		setValue(s,row, column,number, 1);
		if (solveSudoku(s,row,column+1)) return 1;
		setValue(s,row,column, number, 0);
		s->matrix[row][column] = 0; 
	}
	else {
		for(int i = 1; i < eliminationValues[0] + 1; i++) {	
			
			int number = eliminationValues[i];
			s->matrix[row][column] = number;
			setValue(s,row, column,number, 1);

			if (i == eliminationValues[0]) {
				if (solveSudoku(s,row,column+1)) return 1;
			} else {

				int flag = 0;
				MPI_Status status;
				int source = (me == 0) ? nprocs - 1 : (me - 1) % nprocs;
				MPI_Iprobe(source, 1, MPI_COMM_WORLD, &flag, &status);

				if(flag) {

					int needWork;
					MPI_Recv(&needWork, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
					if (needWork == -1) { 
						if (me == 0) {
							recvMatrix(MPI_ANY_SOURCE, s->matrix);
						}
						MPI_Send(&needWork, 1, MPI_INT, (me + 1) % nprocs, 1, MPI_COMM_WORLD);  
						return 1;
					}
					if (needWork == 1) {
							
						int haveWork = 1;
						MPI_Send(&haveWork, 1, MPI_INT, source, 6, MPI_COMM_WORLD);

						sendMatrix(s->matrix, source);
						sendValues(s->rowvalues, source, 1);
						sendValues(s->columnvalues, source, 2);
						sendValues(s->sectionvalues, source, 3);

					}

				
					
				} else if(sendWork) {

					int haveWork = 1;
					MPI_Send(&haveWork, 1, MPI_INT, source, 6, MPI_COMM_WORLD);

					sendMatrix(s->matrix, source);
					sendValues(s->rowvalues, source, 1);
					sendValues(s->columnvalues, source, 2);
					sendValues(s->sectionvalues, source, 3);
					sendWork--;
				} else {
					if (solveSudoku(s,row,column+1)) return 1;	
				}
			}
			setValue(s,row,column, number, 0);
			s->matrix[row][column] = 0;
		}
	}
	free(eliminationValues);
	return 0;
}

int sudokuComunication2(state *s) {
	int me, nprocs;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
	int needWork;
	int IneedWork;
	MPI_Request request, HaveWorkrequest, HaveWorkRecvrequest;
	MPI_Status status, HaveWorkstatus;


	if (me == 0) {
		IneedWork = 0;
	} else {
		IneedWork = 1;
		int workLoad = 1;
		MPI_Send(&workLoad, 1, MPI_INT, (me + 1) % nprocs, 1, MPI_COMM_WORLD);
	}
	int source = (me == 0) ? nprocs - 1 : (me - 1) % nprocs;
	while(1){
		MPI_Status statusBack, statusFront;
		if (IneedWork == 0) {
			if (solveSudoku(s,0,0)) {
				return 1;
			}
			else {
				IneedWork = 1;
				int workLoad = 1;
				if (nprocs == 1) {
					return 0;
				}
				MPI_Send(&workLoad, 1, MPI_INT, (me + 1) % nprocs, 1, MPI_COMM_WORLD);
			}
		}

		int flagFront, flagBack;
		MPI_Iprobe(source, MPI_ANY_TAG, MPI_COMM_WORLD, &flagBack, &statusBack);
 		MPI_Iprobe((me + 1) % nprocs, MPI_ANY_TAG, MPI_COMM_WORLD, &flagFront, &statusFront);	


		if(flagBack) {
			if (statusBack.MPI_TAG == 1) {
				MPI_Recv(&needWork, 1, MPI_INT, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				if (needWork == -2) {
					IneedWork = -2;
					MPI_Send(&IneedWork, 1, MPI_INT, (me + 1) % nprocs, 1, MPI_COMM_WORLD);
					return 0;
				} else {
					IneedWork = needWork + 1;
					if (needWork == -1) {
						if (me == 0) {
							recvMatrix(MPI_ANY_SOURCE, s->matrix);
						}
						MPI_Send(&needWork, 1, MPI_INT, (me + 1) % nprocs, 1, MPI_COMM_WORLD);
						return 1;
					} 		
					int workLoad = IneedWork;
					MPI_Send(&workLoad, 1, MPI_INT, (me + 1) % nprocs, 1, MPI_COMM_WORLD);
				}
			}
			if (IneedWork >= nprocs) {
				IneedWork = -2;
				int EndWork;
				MPI_Send(&IneedWork, 1, MPI_INT, (me + 1) % nprocs, 1, MPI_COMM_WORLD);
				MPI_Recv(&EndWork, 1, MPI_INT, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				return 0;
			}
		}


		if(flagFront) {
			if (statusFront.MPI_TAG == 6) {
				int recvWork;
				MPI_Recv(&recvWork, 1, MPI_INT, (me + 1) % nprocs, 6, MPI_COMM_WORLD, &HaveWorkstatus);
				recvMatrix(HaveWorkstatus.MPI_SOURCE, s->matrix);
				recvValues(HaveWorkstatus.MPI_SOURCE, 1, s->rowvalues);
				recvValues(HaveWorkstatus.MPI_SOURCE, 2, s->columnvalues);
				recvValues(HaveWorkstatus.MPI_SOURCE, 3, s->sectionvalues);
				if(!sendWork)
					sendWork = (IneedWork > 1);
				IneedWork = 0;
			}
		}

	}
}


int main(int argc, char * argv[]) {
	if (argc < 2) {
		int x = 0;
		printf("Wrong usage: needs input file");
	} else {

		int me, nprocs;
    	MPI_Init(&argc, &argv);
    	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    	MPI_Comm_rank(MPI_COMM_WORLD, &me);

		if(me == 0) {

			//Open file
			FILE *file;
			file = fopen(argv[1], "r");

			//Initialization of L dimension and totalsize			
			char * line = NULL;
			size_t size = 0;
			getline(&line, &size, file);
			L = strtol(line,&line,10);

			MPI_Bcast(&L, 1, MPI_INT, 0, MPI_COMM_WORLD);

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

			for(int i = 0; i < dimension; i++) {
				for(int j = 0; j < dimension; j++) {
					MPI_Bcast(possibleValues[i][j], dimension, MPI_CHAR, 0, MPI_COMM_WORLD);
				}
			}

			//Main cycle



			if (sudokuComunication2(&initialstate)) {
				printMatrix(initialstate.matrix);
			} else {
				printf("No solution\n");
			} 
			MPI_Finalize();

		}
		else{ //not thread 0

			//Initialization

			MPI_Bcast(&L, 1, MPI_INT, 0, MPI_COMM_WORLD);
			dimension = L*L;
			totalsize = dimension * dimension;
	
	
			possibleValues = (char***) malloc(dimension*sizeof(char**));
			for (int i = 0; i < dimension; ++i)
			{
				possibleValues[i] = (char**) malloc(dimension*sizeof(char*));

				for (int j = 0; j < dimension; ++j)
				{
					possibleValues[i][j] = (char*) malloc(dimension*sizeof(char));

				}
			}

			for(int i = 0; i < dimension; i++) {
				for(int j = 0; j < dimension; j++) {
					MPI_Bcast(possibleValues[i][j], dimension, MPI_CHAR, 0, MPI_COMM_WORLD);
				}
			}

			//Request Work

			

			
			state s2;

			s2.matrix = (int**) malloc(dimension*sizeof(int*));
			s2.rowvalues = (char**) malloc(dimension*sizeof(char*));
			s2.columnvalues = (char**) malloc(dimension*sizeof(char*));
			s2.sectionvalues = (char**) malloc(dimension*sizeof(char*));
			for (int i = 0; i < dimension; i++) {
				s2.matrix[i] = (int*) malloc(dimension*sizeof(int));
				s2.rowvalues[i] = (char*) malloc(dimension*sizeof(char));
				s2.columnvalues[i] = (char*) malloc(dimension*sizeof(char));
				s2.sectionvalues[i] = (char*) malloc(dimension*sizeof(char));
			}

			
			sudokuComunication2(&s2);

			MPI_Finalize();
		}

  	}

	return 0; 
}


/* TAG values:
	1 : request work (dont have / have)
	2 : send matrix
	3 : send rowvalues
	4 : send columnvalues
	5 : send sectionvalues
	6 : receive work
*/
