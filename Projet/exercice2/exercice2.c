#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define M		6	// Nombre de bits
// Nombre de pairs dynamique !

#include "exercice2.h"

// Variables propres à chaque pair
static pair this;
static pair fingers[M];
static MPI_Status status;
static int leader;
static int est_initiateur;
static int voisin_gauche;
static int voisin_droite;
static sinclair_round round;
static int nb_pairs = 0;
static int run = 1;

static inline void election(int source)
{
	static int nb_round_gagnes = 0;
	static int nb_round_termines = 0;
	static int k = 0;

	MPI_Recv(&round, 1, MPI_SINCLAIR, status.MPI_SOURCE, TAG_ELECTION, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	if(round.initiateur == this.rang)
	{
		if(round.distance == -1){
		// (ce message arrive deux fois : du voisin de gauche et de droite)
		// Le pair courrant a été battu, il n'est plus initiateur.
			est_initiateur = 0;
		}
		else if(round.distance ==0)
		{
			if(++nb_round_termines == 2) {
				nb_round_termines = 0;
				round.initiateur = this.rang;
				round.distance = 1 << ++k;
				MPI_Send(&round, 1, MPI_SINCLAIR, voisin_gauche, TAG_ELECTION, MPI_COMM_WORLD);
				MPI_Send(&round, 1, MPI_SINCLAIR, voisin_droite, TAG_ELECTION, MPI_COMM_WORLD);
			}
		}
		else { // cas où distance >=1
			// (ce message arrive deux fois : du voisin de gauche et de droite)
			// On a gagné l'élection car on n'a pas été battu et on a déjà parcouru tout l'anneau.
			if(++nb_round_gagnes == 2) {
				// Nombre de pairs sur l'anneau
				nb_pairs = (1<<k) - round.distance + 1;

				// On envoie à notre voisin de droite le nombre de pairs sur l'anneau avec le tag TAG_LISTPAIRS_BUILD.
				MPI_Send(&nb_pairs, 1, MPI_INT, voisin_droite, TAG_LISTPAIRS_BUILD, MPI_COMM_WORLD);

				// On construit un tableau de taille nb_pairs où l'on place notre id chord
				// à la position correspondante à notre rang mpi.
				int pairs_chordid[nb_pairs];
				pairs_chordid[this.rang] = this.chordid;

				// On envoie ce tableau à notre voisin de droite avec le tag TAG_LISTPAIRS_BUILD.
				MPI_Send(&pairs_chordid, nb_pairs, MPI_INT, voisin_droite, TAG_LISTPAIRS_BUILD, MPI_COMM_WORLD);

				printf("Le pair {%d,%d} a été élu !\n", this.rang, this.chordid);
			}
		}
	} else { // le cas où l'initiateur n'est pas le pair courrant
		if(round.distance == -1 || round.distance == 0) {
			// L'initiateur a déjà été battu ou a gagné le round, on transmet l'information au 
			// voisin qui n'a pas émi ce message.
			MPI_Send(&round, 1, MPI_SINCLAIR, (source==voisin_droite)?voisin_gauche:voisin_droite, TAG_ELECTION, MPI_COMM_WORLD);
		}
		else { // cas où distance >= 1
			if(round.initiateur < leader) {
				// On envoie à l'emetteur du message que l'initiateur a été battu (donc distance=-1, initiateur a la même valeur).
				round.distance = -1;
				MPI_Send(&round, 1, MPI_SINCLAIR, source, TAG_ELECTION, MPI_COMM_WORLD);
			}
			else if(round.initiateur >= leader) {
				if(round.initiateur > leader) {
					// L'initiateur devient le nouveau leader du pair.
					leader = round.initiateur;
				}

				if(round.distance == 1) {
					// Le round est terminé, on envoie à l'emetteur du message que le round est terminé sans être 
					// battu (distance = 0).
					round.distance = 0;
					MPI_Send(&round, 1, MPI_SINCLAIR, source, TAG_ELECTION, MPI_COMM_WORLD);
				} else { // cas où distance > 1
					// Le round n'est ni terminé, ni battu on envoie donc l'information au voisin n'ayant pas émi ce message avec
					// distance = distance - 1.
					round.distance -= 1;
					MPI_Send(&round, 1, MPI_SINCLAIR, (source==voisin_droite)?voisin_gauche:voisin_droite, TAG_ELECTION, MPI_COMM_WORLD);
				}
			}
		}
	}
}

static inline void construction_liste_fingers(int* pairs)
{
	for(int j=0;j<M;j++)
	{
		int id = (this.chordid + (1 << j))%(1 << M);
		// On cherche le pair qui gère id
		for(int a=0;a<nb_pairs; a++)
		{
			// prédécesseur de pair[k] (ordre cyclique)
			int* pair_precedent = &pairs[(nb_pairs+a-1)%nb_pairs];

			// Si id appartient à l'intervalle ] pairs(k-1) ; pairs(k) ] alors pair[k] est son gestionnaire
			if( dans_intervalle_b_inclus(id, *pair_precedent, pairs[a]) ) {
				fingers[j].rang = a;
				fingers[j].chordid = pairs[a];
				break;
			}
		}
	}

	char print[250] = "";
	for(int i=0;i<M;i++) sprintf(print,"%s{%d,%d} ", print, fingers[i].rang, fingers[i].chordid);
	printf("Fingers de {%d,%d} : %s\n", this.rang, this.chordid, print);
}

static inline void construction_liste_pairs()
{
	if(this.rang == leader) // Le leader reçoit la liste complète
	{
		int pairs_chordid[nb_pairs];
		MPI_Recv(&pairs_chordid, nb_pairs, MPI_INT, MPI_ANY_SOURCE, TAG_LISTPAIRS_BUILD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		// Le leader possède la liste complète des pairs, il doit la communiquer à tous.
		MPI_Send(&pairs_chordid, nb_pairs, MPI_INT, voisin_droite, TAG_LISTPAIRS_COMPLETE, MPI_COMM_WORLD);

		// Le leader connait tous les pairs donc il peut construire sa table de fingers.
		construction_liste_fingers(pairs_chordid);
		run = 0;
	}
	else
	{
		MPI_Recv(&nb_pairs, 1, MPI_INT, MPI_ANY_SOURCE, TAG_LISTPAIRS_BUILD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		int pairs_chordid[nb_pairs];
		MPI_Recv(&pairs_chordid, nb_pairs, MPI_INT, MPI_ANY_SOURCE, TAG_LISTPAIRS_BUILD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		pairs_chordid[this.rang] = this.chordid;

		if(voisin_droite != leader)
		{
			MPI_Send(&nb_pairs, 1, MPI_INT, voisin_droite, TAG_LISTPAIRS_BUILD, MPI_COMM_WORLD);
		}
		MPI_Send(&pairs_chordid, nb_pairs, MPI_INT, voisin_droite, TAG_LISTPAIRS_BUILD, MPI_COMM_WORLD);
	}
}


void main_pair()
{
	// Affectations des valeurs locales
	MPI_Recv(&this, 1, MPI_PAIR, MPI_ANY_SOURCE, TAG_INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&est_initiateur, 1, MPI_INT, MPI_ANY_SOURCE, TAG_INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&voisin_gauche, 1, MPI_INT, MPI_ANY_SOURCE, TAG_INIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&voisin_droite, 1, MPI_INT, MPI_ANY_SOURCE, TAG_INIT, MPI_COMM_WORLD, &status);
	leader = -1;

	// Attend que tous les pairs soient initialisés
	printf("{%d,%d} est initialisé %s\n", this.rang, this.chordid, (est_initiateur)?"et participe à l'élection !":"!");
	MPI_Barrier(MPI_COMM_WORLD);

	if(est_initiateur) { // si un est initiateur, on lance l'élection
		leader=this.rang;
		round.initiateur = this.rang;
		round.distance = 1;
		MPI_Send(&round, 1, MPI_SINCLAIR, voisin_gauche, TAG_ELECTION, MPI_COMM_WORLD);
		MPI_Send(&round, 1, MPI_SINCLAIR, voisin_droite, TAG_ELECTION, MPI_COMM_WORLD);
	}

	while(run) {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG,MPI_COMM_WORLD, &status);

		switch(status.MPI_TAG) {
			case TAG_ELECTION :
				election(status.MPI_SOURCE);
				break;
			case TAG_LISTPAIRS_BUILD :
				construction_liste_pairs();
				break;
			case TAG_LISTPAIRS_COMPLETE :
				{
					int pairs_chordid[nb_pairs];
					MPI_Recv(&pairs_chordid, nb_pairs, MPI_INT, MPI_ANY_SOURCE, TAG_LISTPAIRS_COMPLETE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					construction_liste_fingers(pairs_chordid);
					if(voisin_droite != leader) {
						MPI_Send(&pairs_chordid, nb_pairs, MPI_INT, voisin_droite, TAG_LISTPAIRS_COMPLETE, MPI_COMM_WORLD);
					}
					run = 0;
				}
				break;
		}
	}
}

void simulateur(int nb_pairs)
{
	srand(time(NULL));
	init_ensemble_I();

	pair pairs[nb_pairs];
	int initiateurs[nb_pairs];
	int nb_initiateurs = 0;

	// Tirage aléatoire des identifiants des pairs et des initiateurs
	for(int i=0;i<nb_pairs;i++) {
		pairs[i].chordid = f(i);
		if(rand()%3==0) { // 1 chance sur 3
			initiateurs[i]=1;
			nb_initiateurs++;
		}
		else initiateurs[i]=0;
	}
	initiateurs[rand()%nb_pairs] = 1;
	if(!nb_initiateurs) initiateurs[rand()%nb_pairs] = 1; // Force au moins 1 initiateur
	qsort(&pairs, nb_pairs, sizeof(pair), &compare_pair); // tri par ordre croissant
	for(int i=0;i<nb_pairs;i++) pairs[i].rang = i; // le rang mpi sans 0
	
	// On communique à chaque pair son rang, son id chord et ses voisins
	for(int i=0;i<nb_pairs;i++) {
		int gauche = (pairs[i].rang==0)?nb_pairs-1:(i-1);
		int droite = (pairs[i].rang==nb_pairs-1)?0:(i+1);

		MPI_Send(&pairs[i], 1, MPI_PAIR, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
		MPI_Send(&initiateurs[i], 1, MPI_INT, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
		MPI_Send(&gauche, 1, MPI_INT, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
		MPI_Send(&droite, 1, MPI_INT, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
	}

	// On attend que tous les pairs soient initialisés.
	MPI_Barrier(MPI_COMM_WORLD);
}


/******************************************************************************/

int main(int argc, char* argv[])
{
	int nb_proc, rang;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

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

	if (rang == nb_proc-1) simulateur(nb_proc-1); // Le dernier proc est le simulateur
	else main_pair();

	MPI_Finalize();
	return 0;
}
