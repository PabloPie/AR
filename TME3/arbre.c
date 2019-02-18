#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define TAGINIT 0
#define TAGCALC 1
#define TAGDECI 2
#define NB_SITE 6
#define INITIATOR 0

int nb_voisins, local_value, val_recv;
int* voisins;
int voisins_recu[NB_SITE + 1];

void simulateur(void) {
   int i;

   /* nb_voisins[i] est le nombre de voisins du site i */
   int nb_voisins[NB_SITE+1] = {-1, 2, 3, 2, 1, 1, 1};
   int min_local[NB_SITE+1] = {-1, 3, 11, 8, 14, 5, 17};

   /* liste des voisins */
   int voisins[NB_SITE+1][3] = {{-1, -1, -1},
		 {2, 3, -1}, {1, 4, 5}, 
		 {1, 6, -1}, {2, -1, -1},
		 {2, -1, -1}, {3, -1, -1}};
							   
   for(i=1; i<=NB_SITE; i++){
	  MPI_Send(&nb_voisins[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);    
	  MPI_Send(voisins[i],nb_voisins[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
	  MPI_Send(&min_local[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD); 
   }
}

void initialize_values()
{  
   MPI_Status status;
   MPI_Recv(&nb_voisins, 1, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
   voisins = (int *)malloc(sizeof(int)*nb_voisins);
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

void calcul_min(int rang)
{
	int nb_msg_recv = 0;
	int parent, i = 0;

	MPI_Status status;
	while (nb_msg_recv < nb_voisins - 1){
		MPI_Recv(&val_recv, 1, MPI_INT, MPI_ANY_SOURCE, TAGCALC, MPI_COMM_WORLD, &status);
		voisins_recu[status.MPI_SOURCE] = 1;
		if (val_recv < local_value) {
			local_value = val_recv;
		}
		nb_msg_recv++;
	}

	for (i = 0; i < nb_voisins; i++ ) {
		if (voisins_recu[voisins[i]] == 0) {
			parent = voisins[i];
		}
	}
	printf("Process %d sending min value %d to its parent %d", rang, local_value, parent);
	MPI_Send(&local_value, 1, MPI_INT, parent, TAGCALC, MPI_COMM_WORLD);
	MPI_Recv(&val_recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	if (status.MPI_TAG == TAGDECI) {
		// on est pas decideur
		// listen and propagate to neighbor except status.MPI_SOURCE
	} else (status.MPI_TAG == TAGCALC){
		// on est decideur
		// propagate value
	}

}

/******************************************************************************/

int main (int argc, char* argv[]) {
   int nb_proc,rang;
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
	  // show_initial(rang);
	  calcul_min(rang);
   }
  
   MPI_Finalize();
   return 0;
}
