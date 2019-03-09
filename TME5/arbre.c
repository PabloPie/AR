#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#define NB_SITE 6
#define INITIATOR 0

#define TAGINIT	0
#define TAGREVEIL 1
#define TAGIDENT 2
#define TAGLEADER 3


static int nb_voisins, rang, initiateur, local_value, reveil, leader, recv, ident_recues, parent;
static int* voisins;
static int voisins_recu[NB_SITE + 1];

void simulateur(void) {
   int i;

   /* nb_voisins[i] est le nombre de voisins du site i */
   int nb_voisins[NB_SITE+1] = {-1, 2, 3, 2, 1, 1, 1};

   /* liste des voisins */
   int voisins[NB_SITE+1][3] = {{-1, -1, -1},
		 {2, 3, -1}, {1, 4, 5}, 
		 {1, 6, -1}, {2, -1, -1},
		 {2, -1, -1}, {3, -1, -1}};
							   
   for(i=1; i<=NB_SITE; i++){
	  MPI_Send(&nb_voisins[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);    
	  MPI_Send(voisins[i],nb_voisins[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
   }
}

// Initialise les valeurs locales
void initialize_values()
{  
	MPI_Status status;
	MPI_Recv(&nb_voisins, 1, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
	voisins = (int *)malloc(sizeof(int)*nb_voisins);
	MPI_Recv(voisins, nb_voisins, MPI_INT, INITIATOR, TAGINIT, MPI_COMM_WORLD, &status);
	local_value = rang;
	reveil = 0;
	leader = -1;
	srand(getpid());
	initiateur = rand()%2;
}

static inline void envoyer(int voisin, int val, int tag)
{
	MPI_Send(&val,1,MPI_INT,voisin,tag,MPI_COMM_WORLD);
}

static inline void main_election()
{
	int i;
	
	if(initiateur)
	{
		reveil=1;
		printf("%d> Je suis réveillé et initiateur.\n", rang);
		if(nb_voisins==1) envoyer(voisins[0],rang,TAGIDENT); // Si on est une feuille on envoit directement notre id
		else for(i=0;i<nb_voisins;i++) // Sinon on réveille nos voisins
		{
			if(voisins[i]!=rang) envoyer(voisins[i],0,TAGREVEIL);
		}
	}
	
	MPI_Status status;
	while(leader==-1)
	{
		MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if(reveil==0 && (status.MPI_TAG == TAGREVEIL || status.MPI_TAG == TAGIDENT))
		{ // Si on est pas réveillé et qu'on reçoit un TAGREVEIL ou TAGIDENT = il y a election
			reveil = 1;
			printf("%d> Je suis réveillé.\n",rang);
			if(nb_voisins==1) envoyer(voisins[0],rang,TAGIDENT); // Si on est une feuille on envoit directement notre id
			else for(i=0;i<nb_voisins;i++) // Sinon on réveille nos voisins
			{
				if(voisins[i]!=status.MPI_SOURCE) envoyer(voisins[i],0,TAGREVEIL);
			}
		}
		
		if(status.MPI_TAG == TAGIDENT)
		{
			// On met à jour notre identité locale
			if(recv < local_value) local_value = recv;
			voisins_recu[status.MPI_SOURCE]=1;
			
			if(++ident_recues == nb_voisins -1)
			{ // Si on a reçu une réponse de tous nos voisins sauf un alors on lui envoit
				for(i=0;i<nb_voisins;i++) // On cherche notre parent
					if(voisins_recu[voisins[i]]==0) parent=voisins[i];
				envoyer(parent,local_value,TAGIDENT); // On envoit la meilleure id connue
			}
			else if(ident_recues == nb_voisins)
			{ // Si on a reçu un message de tous nos voisins alors on connait la meilleure id
				leader = local_value;
				for(i=0;i<nb_voisins;i++) // On la transmet à nos autres voisins
					if(voisins[i]!=status.MPI_SOURCE) envoyer(voisins[i],leader,TAGIDENT);
			}
		}
	}
	
	printf("%d> le leader est %d !\n",rang, leader);
}

int main(int argc, char* argv[])
{
	int nbNoeuds;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nbNoeuds);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	
	if (rang == INITIATOR) {
		simulateur();
	} else {
		initialize_values();
		main_election();
	}
	MPI_Finalize();
	free(voisins);
}
