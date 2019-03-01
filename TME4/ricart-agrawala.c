#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define REQ 0
#define REPLY 1

#define MAX_CS 5

#define NON_DEM 0
#define DEM 1
#define EN_SC 2

int rank, nb_proc;

int etat;
int nb_reply;
int date_req;
int h;
int pending_req[];
int last_index = 0;

int count_end = 0;

struct msg
{
	int TAG;
	int horloge;
	int id;
} mesg_recv, mesg_sent;

inline int max(int a, int b) {
   return (a>b?a:b);
}

void sc() {
	sleep(5);
}

void request_sc()
{
	h++;
	date_req = h;
	etat = DEM;
	nbreply = 0;
	// construire message à envoyer
	MPI_Bcast(&mesg, 3, MPI_INT, rank, MPI_COMM_WORLD);
	while (etat != EN_SC)
		reception();
}

void release_sc()
{
	h++;
	int i;
	//construire mesage a envoyer
	for (i = 0; i <= last_index; i++)
		MPI_Send(&mesg_sent, 3, MPI_INT, pending_req[i], 0, MPI_COMM_WORLD);
	etat = NON_DEM;
	last_index = 0;
}

void reception()
{	
	MPI_Status status;
	MPI_Recv(&mesg_recv, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	h = max(h, hj) +1;
	switch (mesg_recv.TAG) {

		case REQ :
			if (etat == NON_DEM || mesg_recv.horloge < h || (mesg_recv.horloge == h && mesg_recv.id < rank)) {
				h++;
				//construire message a envoyer
				MPI_Send(&msg, 3, MPI_INT, mesg.id, 0, MPI_COMM_WORLD);
			} else {
				pending_req[last_index] = mesg.id;
			}
		
		case REPLY :
			nb_reply++;
			if (nb_reply == nb_proc - 1)
				etat = EN_SC;
		
		case END :
			count_end++;
			if (count_end == nb_proc)
				end = 1;
	}
}

/******************************************************************************/

int main (int argc, char* argv[]) {
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
   MPI_Comm_rank(MPI_COMM_WORLD, &rang);
  
	while (cont < MAX_CS) {
		request_sc();
		sc();
		release_sc();
	}

	while (count_end < nb_proc -1)
		reception();



   MPI_Finalize();
   return 0;
}
