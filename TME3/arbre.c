#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define TAGINIT 0
#define TAGCALC 1
#define TAGDECI 2
#define NB_SITE 6
#define INITIATOR 0

static int nb_voisins, local_value, val_recv, rang;
static int* voisins;
static int voisins_recu[NB_SITE + 1];

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

// Initialise les valeurs locales envoyées par le processus 0
void initialize_values()
{  
   MPI_Status status;
   MPI_Recv(&nb_voisins, 1, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
   voisins = (int *)malloc(sizeof(int)*nb_voisins);
   MPI_Recv(voisins, nb_voisins, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
   MPI_Recv(&local_value, 1, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
}

// Garde la meilleure valeur entre valeur reçue et valeur locale
void update_value()
{
	// plus petite valeur
	if(val_recv < local_value) local_value = val_recv;
	// plus grande valeur
	//if(val_recv > local_value) local_value = val_recv;
}

void calcul_min()
{
	int parent, i, nb_msg_recv = 0;
	MPI_Status status;

	// On attend un message de tous nos voisins sauf un
	while (nb_msg_recv < nb_voisins - 1){
		MPI_Recv(&val_recv, 1, MPI_INT, MPI_ANY_SOURCE, TAGCALC, MPI_COMM_WORLD, &status);
		update_value(); // on met à jour notre valeur locale
		voisins_recu[status.MPI_SOURCE] = 1;
		nb_msg_recv++;
	}

	// Qui est notre parent ? (aka celui qui n'a pas envoyé de message)
	for (i = 0; i < nb_voisins; i++ ){
		if (voisins_recu[voisins[i]] == 0)
			parent = voisins[i];
	}
	
	// On envoit notre valeur locale à notre parent
	MPI_Send(&local_value, 1, MPI_INT, parent, TAGCALC, MPI_COMM_WORLD);

	// On attend un message et on met à jour notre valeur locale
	MPI_Recv(&val_recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	update_value();

	// Si c'est un TAGCALC alors on a reçu un message de tous les voisins
	// donc on est un décideur
	if(status.MPI_TAG == TAGCALC){
		printf("Process %d is a leader.\n",rang);
		for (i = 0; i < nb_voisins; i++ ) {
			MPI_Send(&local_value, 1, MPI_INT, voisins[i], TAGDECI, MPI_COMM_WORLD);
		}
	}
	// Sinon c'est un message venant d'un décideur donc on transmet aux 
	// autres voisins.
	else{ 
		for (i = 0; i < nb_voisins; i++){
			if(voisins[i]!=status.MPI_SOURCE)
				MPI_Send(&local_value, 1, MPI_INT, voisins[i], TAGDECI, MPI_COMM_WORLD);
		}
	}
	
	printf("Process %d ends with the value %d.\n",rang, local_value);
}

/******************************************************************************/

int main (int argc, char* argv[]) {
   int nb_proc;
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

   if (nb_proc != NB_SITE+1) {
	  printf("Nombre de processus incorrect, il en faut %d !\n", NB_SITE+1);
	  MPI_Finalize();
	  return 0;
   }
  
   MPI_Comm_rank(MPI_COMM_WORLD, &rang);
  
   if (rang == INITIATOR) {
	  simulateur();
   } else {
	  initialize_values();
	  calcul_min(rang);
   }
  
   MPI_Finalize();
   free(voisins);
   return 0;
}
