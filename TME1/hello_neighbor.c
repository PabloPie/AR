#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#define SIZE 128

int main(int argc, char **argv){
	int my_rank;
	int nb_proc;
	int source;
	int dest;
	int tag =0;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	char message[SIZE];
	char message_rcv[SIZE];

	sprintf(message, "Hello %d from %d", (my_rank+1)%nb_proc, my_rank);
	dest = (my_rank+1)%nb_proc;
	source = (my_rank-1)%nb_proc;
	if(my_rank==0){
		MPI_Ssend(message, strlen(message)+1, MPI_CHAR,dest,tag,MPI_COMM_WORLD);
		MPI_Recv(message, SIZE, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
		printf("%s\n", message);
	} else {
		MPI_Recv(message_rcv, SIZE, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
		MPI_Ssend(message, strlen(message)+1, MPI_CHAR,dest,tag,MPI_COMM_WORLD);
		printf("%s\n", message_rcv);
	}

	MPI_Finalize();
	return 0;
}
