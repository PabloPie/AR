#include "mpi_server.h"

#define SIZE 128

int my_rank, nb_proc, source, dest, tag;
MPI_Status status;
char message[SIZE];
char message_rcv[SIZE];

void prepare_message()
{
	sprintf(message, "Hello %d from %d", (my_rank+1)%nb_proc, my_rank);
	tag = 0;
	dest = (my_rank+1)%nb_proc;
	source = (my_rank-1)%nb_proc;
}

void msg_callback(int _tag, int _source)
{
	printf("Process %d: Message received.", my_rank);
	prepare_message();
	
	pthread_mutex_lock(getMutex());
	MPI_Recv(message_rcv, SIZE, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	pthread_mutex_unlock(getMutex());

	pthread_mutex_lock(getMutex());
	MPI_Send(message, strlen(message)+1, MPI_CHAR, dest, MPI_ANY_TAG, MPI_COMM_WORLD);
	pthread_mutex_unlock(getMutex());
	printf("%s\n", message_rcv);
}

int main(int argc, char ** argv)
{
	int var;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &var);
	// MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);

	if (my_rank == 0){
		prepare_message();
		MPI_Send(message, strlen(message)+1, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
	}
    
    start_server(msg_callback);

    pthread_cond_wait(getCond(),getMutex());
	printf("Received signal, %d closing.", my_rank);
    destroy_server();
	MPI_Finalize();
}
