#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAGINIT 0
#define TAGCALC 1
#define TAGRESP 2
#define TAGEND 3
#define NB_SITE 6
#define INITIATOR 0
#define ECHO_INIT 1

int nb_voisins, local_value, val_recv;
int* voisins;
int predecessor, rang;
int initiated = 0;
int* next;
int resps = 0;

void simulateur(void) {
	int i;

	/* nb_voisins[i] est le nombre de voisins du site i */
	int nb_voisins[NB_SITE+1] = {-1, 3, 3, 2, 3, 5, 2};
	int min_local[NB_SITE+1] = {-1, 1, 11, 8, 14, 5, 17};

   /* liste des voisins */
	int voisins[NB_SITE+1][5] = {{-1, -1, -1, -1, -1},
		 {2, 5, 3, -1, -1}, {4, 1, 5, -1, -1},
		 {1, 5, -1, -1, -1}, {6, 2, 5, -1, -1},
		 {1, 2, 6, 4, 3}, {4, 5, -1, -1, -1}};

	for(i=1; i<=NB_SITE; i++){
		MPI_Send(&nb_voisins[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
		MPI_Send(voisins[i], nb_voisins[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
		MPI_Send(&min_local[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
	}
}

void initialize_values()
{
	MPI_Status status;
	MPI_Recv(&nb_voisins, 1, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
	voisins = (int *)malloc(sizeof(int)*nb_voisins);
	next = (int *)malloc(sizeof(int)*nb_voisins);
	MPI_Recv(voisins, nb_voisins, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&local_value, 1, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
}

void show_initial(int rang)
{
	int i = 0;

	printf("Processus de rang %d\n========\n",rang);
	printf("Nb voisins: %d\n", nb_voisins);
	for (; i < nb_voisins; i++)
		printf("Voisin %d: %d\n", i, voisins[i]);
	printf("Initial local value: %d\n\n", local_value);
}

static inline void set_min()
{
	local_value = val_recv < local_value ? val_recv : local_value;
}

void send_msg()
{
	int i;
	for (i = 0; i < nb_voisins; i++) {
		if (voisins[i] != predecessor) // not really optimal
			MPI_Send(&local_value, 1, MPI_INT, voisins[i], TAGCALC, MPI_COMM_WORLD);
	}
}

void receive_msg()
{
	MPI_Status status;
	int nb_responses = 0;
	while (nb_responses < nb_voisins - 1){
		MPI_Recv(&val_recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (TAGCALC && !initiated) {
			initiated = 1;
			predecessor = status.MPI_SOURCE;
			printf("%d received request for calc from %d\n", rang, predecessor);
			set_min();
			send_msg();
		} else if(initiated && status.MPI_TAG == TAGRESP) {
			next[resps] = status.MPI_SOURCE;
			resps++;
			nb_responses++;
			set_min();
		} else if (initiated && status.MPI_TAG == TAGCALC) {
			nb_responses++;
			set_min();
		}
	}
	if (rang != ECHO_INIT) {
		int i;
		printf("All received, sending resp to %d, from %d\n", predecessor, rang);
		for (i = 0; i < resps; i++)
			printf("%d :fils %d: %d\n", rang, i, next[i]);
		MPI_Send(&local_value, 1, MPI_INT, predecessor, TAGRESP, MPI_COMM_WORLD);
		MPI_Recv(&local_value, 1, MPI_INT, predecessor, TAGEND, MPI_COMM_WORLD, &status);
		for (i = 0; i < resps; i++)
			MPI_Send(&local_value, 1, MPI_INT, next[i], TAGEND, MPI_COMM_WORLD);
		printf("%d : min value is %d\n", rang, local_value);
	}
}

void calcul_min(int rang)
{
	int i;
	predecessor = rang;
	if (rang == ECHO_INIT){
		initiated = 1;
		send_msg();
		receive_msg();
		printf("%d is the min value, sending back...\n", local_value);
		for (i = 0; i < nb_voisins; i++) {
			printf("%d :fils %d: %d\n", rang, i, voisins[i]);
			MPI_Send(&local_value, 1, MPI_INT, voisins[i], TAGEND, MPI_COMM_WORLD);
		}
	} else {
		receive_msg();
		printf("%d ended\n", rang);
	}
}
/******************************************************************************/

int main (int argc, char* argv[]) {
	int nb_proc;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

	if (nb_proc != NB_SITE+1) {
		printf("Nombre de processus incorrect !\n");
		MPI_Finalize();
		exit(2);
	}

	MPI_Comm_rank(MPI_COMM_WORLD, &rang);

	if (rang == INITIATOR) {
		simulateur();
	} else {
		initialize_values();
		calcul_min(rang);
	}

	MPI_Finalize();
	return 0;
}
