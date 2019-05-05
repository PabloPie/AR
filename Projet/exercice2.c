#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define M		6	// Nombre de bits
#define PAIRS	9	// Nombre de pairs

#include "exercice2.h"

// Variables propres à chaque pair
static pair this;
static pair fingers[M];
static MPI_Status status;
static int leader;
static int est_initiateur;
static int voisin_gauche;
static int voisin_droite;


void main_pair()
{
	// Affectations des valeurs locales
	MPI_Recv(&this, 1, MPI_PAIR, INITIATEUR, TAG_INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&est_initiateur, 1, MPI_INT, INITIATEUR, TAG_INIT, MPI_COMM_WORLD, &status);
	leader = -1;
	voisin_gauche = (this.rang==1)?PAIRS:(this.rang-1);
	voisin_droite = (this.rang==PAIRS)?1:(this.rang+1);

	// Attend que tous les pairs soient initialisés
	printf("{%d,%d} est initialisé %s g=%d d=%d\n", this.rang, this.chordid, (est_initiateur)?"et participe à l'élection !":"!",voisin_gauche, voisin_droite);
	MPI_Barrier(MPI_COMM_WORLD);

	int nb_retours_round = 0;
	int k=0;
	sinclair_round election = {this.rang, 1};

	if(est_initiateur) { // si un est initiateur, on lance l'élection
		leader=this.rang;
		MPI_Send(&election, 1, MPI_SINCLAIR, voisin_gauche, TAG_ELECTION, MPI_COMM_WORLD);
		MPI_Send(&election, 1, MPI_SINCLAIR, voisin_droite, TAG_ELECTION, MPI_COMM_WORLD);
	}
	
	int run=1;
	while(run) {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG,MPI_COMM_WORLD, &status);

		switch(status.MPI_TAG) {
			case TAG_ELECTION :
				MPI_Recv(&election, 1, MPI_SINCLAIR, status.MPI_SOURCE, TAG_ELECTION, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				//printf("de=%d a=%d init=%d dist=%d leader=%d\n",status.MPI_SOURCE,this.rang,election.initiateur, election.distance,leader);
				if(this.rang == election.initiateur)
				{
					if(election.distance == -1)
					{
						//perdu
					}
					else if(election.distance == 0)
					{
						// round suivant
						if(++nb_retours_round == 2)
						{
							nb_retours_round=0;
							election.distance = 1 << ++k;
							MPI_Send(&election, 1, MPI_SINCLAIR, voisin_gauche, TAG_ELECTION, MPI_COMM_WORLD);
							MPI_Send(&election, 1, MPI_SINCLAIR, voisin_droite, TAG_ELECTION, MPI_COMM_WORLD);
						}
					}
					else if(election.distance > 0)
					{
						// on a gagne
						for(int i=1;i<=PAIRS;i++) if(i!=this.rang)MPI_Send(&election, 1, MPI_SINCLAIR, i, TAG_LEADER, MPI_COMM_WORLD);
						run = 0;
					}
					else printf("ERR1 P%d\n",this.rang);
				}
				else
				{
					if(election.distance == -1 || election.distance == 0)
					{
						// initiateur a déjà perdu
						if(status.MPI_SOURCE == voisin_droite)
							MPI_Send(&election, 1, MPI_SINCLAIR, voisin_gauche, TAG_ELECTION, MPI_COMM_WORLD);
						else
							MPI_Send(&election, 1, MPI_SINCLAIR, voisin_droite, TAG_ELECTION, MPI_COMM_WORLD);
					}
					else // dist >= 1
					{
						if(election.initiateur > leader)
						{
							leader = election.initiateur;
							//printf("P%d leader=%d\n", this.rang, leader);
						}
						election.distance--;
						if(election.distance == 0)
						{
							MPI_Send(&election, 1, MPI_SINCLAIR, status.MPI_SOURCE, TAG_ELECTION, MPI_COMM_WORLD);
						}
						else if(election.distance >= 1)
						{
							if(status.MPI_SOURCE == voisin_droite)
								MPI_Send(&election, 1, MPI_SINCLAIR, voisin_gauche, TAG_ELECTION, MPI_COMM_WORLD);
							else
								MPI_Send(&election, 1, MPI_SINCLAIR, voisin_droite, TAG_ELECTION, MPI_COMM_WORLD);
						}
						else printf("ERR2 P%d\n",this.rang);
					}
				}
				
				break;
			case TAG_LEADER :
				run=0;
				break;
			case TAG_LISTPAIRS_BUILD :
				//
				break;
			case TAG_LISTPAIRS_COMPLETE :
				//
				break;
		}
	}
	printf("P%d fin avec pour leader %d\n",this.rang, leader);
}

void simulateur()
{
	srand(time(NULL));
	init_ensemble_I();
	
	
	pair pairs[PAIRS];
	int initiateurs[PAIRS];
	int nb_initiateurs = 0;

	// Tirage aléatoire des identifiants des pairs et des initiateurs
	for(int i=0;i<PAIRS;i++) {
		pairs[i].chordid = f(i);
		if(rand()%3==0) { // 1 chance sur 3
			initiateurs[i]=1;
			nb_initiateurs++;
		}
		else initiateurs[i]=0;
	}
	initiateurs[rand()%PAIRS] = 1;
	if(!nb_initiateurs) initiateurs[rand()%PAIRS] = 1; // Force au moins 1 initiateur
	qsort(&pairs, PAIRS, sizeof(pair), &compare_pair); // tri par ordre croissant
	for(int i=0;i<PAIRS;i++) pairs[i].rang = i+1; // le rang mpi sans 0
	
	// On communique à chaque pair son id chord
	for(int i=0;i<PAIRS;i++) {
		MPI_Send(&pairs[i], 1, MPI_PAIR, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
		MPI_Send(&initiateurs[i], 1, MPI_INT, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
	}

	// On attend que tous les pairs soient initialisés.
	MPI_Barrier(MPI_COMM_WORLD);
}


/******************************************************************************/

int main(int argc, char* argv[])
{
	int nb_proc;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

	if (nb_proc != PAIRS+1) {
		printf("Nombre de processus incorrect, il en faut %d !\n", PAIRS+1);
		MPI_Finalize();
		return 0;
	}

	MPI_Comm_rank(MPI_COMM_WORLD, &rang);

	// Création d'un nouveau type MPI (pour transmettre la structure pair)
	int lengths[] = {1,1};
	MPI_Datatype types[] = {MPI_INT, MPI_INT};
	MPI_Aint offsets[] = {offsetof(pair, rang),offsetof(pair, chordid)};
	MPI_Type_create_struct(2, lengths, offsets, types, &MPI_PAIR);
	MPI_Type_commit(&MPI_PAIR);
	// Création d'un nouveau type MPI (pour transmettre la structure election)
	MPI_Type_create_struct(2, lengths, offsets, types, &MPI_SINCLAIR);
	MPI_Type_commit(&MPI_SINCLAIR);

	if (rang == 0) simulateur();
	else main_pair();

	MPI_Finalize();
	return 0;
}
