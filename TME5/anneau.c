#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>

#define TAGINIT	0
#define TAGWIN 1

static int rang, nbNoeuds, voisin, initiateur;
static int leader = -1;

static inline void envoyer(int val, int tag)
{
	MPI_Send(&val,1,MPI_INT,voisin,tag,MPI_COMM_WORLD);
}

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nbNoeuds);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);
	voisin = (rang+1)%nbNoeuds;

	srand(getpid());
	initiateur = rand()%2;
	if(initiateur)
	{
		envoyer(rang,TAGINIT);
		printf("%d> je suis initiateur.\n", rang);
	}
	
	MPI_Status status;
	int recv;
	while(leader==-1)
	{
		MPI_Recv(&recv, sizeof(int), MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if(status.MPI_TAG == TAGINIT)
		{
			if(initiateur)
			{
				if(recv > rang)
				{ // J'ai été battu donc je ne suis plus initiateur et je transmets
					initiateur = 0;
					envoyer(recv, TAGINIT);
					printf("%d> j'ai été battu par %d.\n",rang,recv);
				}
				else if(recv < rang)
				{ // Je bas un initiateur donc je ne transmets pas son jeton
					printf("%d> je détruis le jeton de %d.\n",rang,recv);
				}
				else if(recv == rang)
				{ // j'ai gagné
					envoyer(rang, TAGWIN);
				}
			}
			else
			{ // Je ne suis pas initiateur et je ne peux plus le devenir donc je transmets
				envoyer(recv, TAGINIT);
			}
		}
		else // TAGWIN
		{
			leader = recv;
			if(leader == rang)
			{
				printf("%d> j'ai gagné et tous le monde le sait !\n",rang);
			}
			else
			{
				envoyer(recv, TAGWIN);
			}
		}
	}
	
	printf("%d> le leader est %d.\n",rang, leader);
	
	

	MPI_Finalize();
}
