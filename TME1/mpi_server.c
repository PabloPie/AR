#include "mpi_server.h"
static server the_server;

void *listen(void *arg)
{
	int flag = 0;
	MPI_Status status;

	while(1){
		pthread_mutex_lock(getMutex());
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
		pthread_mutex_unlock(getMutex());
		if (flag) {
			the_server.callback(status.MPI_TAG, status.MPI_SOURCE);
			pthread_cond_signal(getCond());
		}
	}
}

void start_server(void (*callback)(int, int))
{
	pthread_mutex_init(getMutex(), NULL);
	pthread_create(&the_server.listener, NULL, listen, NULL);
	pthread_cond_init(getCond(), NULL);
	the_server.callback = callback;
}

void destroy_server()
{
	pthread_cancel(the_server.listener);
	pthread_mutex_destroy(getMutex());
	pthread_cond_destroy(getCond());
}

pthread_mutex_t* getMutex()
{
	return &the_server.mutex;
}

pthread_cond_t* getCond()
{
	return &the_server.cv;
}