#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define M		6	// Nombre de bits
#define PAIRS	10	// Nombre de pairs (sans celui qui rejoint)

#define RANG_REJOINDRE	11

#include "exercice3.h"

// pair courant (contient id mpi et id chord)
static pair this;

// liste des fingers du pair
// le premier finger est aussi son successeur
static pair fingers[M];
static pair inverses[PAIRS+1];

void simulateur()
{
	/* DEBUT INITIALISATION DES PAIRS */
	srand(time(NULL));
	init_ensemble_I();			// création de l'ensemble I
	
	pair pairs[PAIRS];			// Liste des pairs
	pair fingers[PAIRS][M];		// Liste des fingers de chaque pair
	pair inverses[PAIRS][PAIRS];

	// Tirage aléatoire des id des pairs
	for(int i=0;i<PAIRS;i++) pairs[i].chordid = f(i);
	qsort(&pairs, PAIRS, sizeof(pair), &compare_pair); // tri par ordre croissant
	for(int i=0;i<PAIRS;i++) pairs[i].rang = i+1; // le rang mpi sans 0

	printf("IDs des pairs : ");
	for(int i=0;i<PAIRS;i++) printf("{%d;%d} ", pairs[i].rang, pairs[i].chordid);
	printf("\n");

	// Mise à zéro des fingers
	for(int i=0;i<PAIRS*M;i++) fingers[i/M][i%M].rang = -1;
	
	// Mise à zéro des inverses
	for(int i=0;i<(PAIRS)*(PAIRS);i++) inverses[i/(PAIRS)][i%(PAIRS)].rang = -1;

	// Calcul des fingers pour chaque pair
	for(int i=0;i<PAIRS;i++)
	{
		// Pour chaque finger de pair[i]
		for(int j=0;j<M;j++)
		{
			int id = (pairs[i].chordid + (1 << j))%(1 << M);
			// On cherche le pair qui gère id
			for(int k=0;k<PAIRS; k++)
			{
				// prédécesseur de pair[k] (ordre cyclique)
				pair* pair_precedent = &pairs[(PAIRS+k-1)%PAIRS];

				// Si id appartient à l'intervalle ] pairs(k-1) ; pairs(k) ] alors pair[k] est son gestionnaire
				if( dans_intervalle_b_inclus(id, pair_precedent->chordid, pairs[k].chordid) ) {
					fingers[i][j] = pairs[k];
					inverses[k][i] = pairs[i];
					break;
				}
			}	
		}
	}
	for(int i=0;i<PAIRS;i++){
		// envoi de l'id local
		MPI_Send(&pairs[i], 1, MPI_PAIR, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
		// envoi des fingers
		MPI_Send(&fingers[i], M, MPI_PAIR, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
		// envoi des inverses
		MPI_Send(&inverses[i], PAIRS+1, MPI_PAIR, pairs[i].rang, TAG_INIT, MPI_COMM_WORLD);
	}

	pair pair_rejoint = {PAIRS+1,f(PAIRS)};
	MPI_Send(&pair_rejoint, 1, MPI_PAIR, PAIRS+1, TAG_INIT, MPI_COMM_WORLD);
	MPI_Send(&pairs[rand()%(PAIRS)], 1, MPI_PAIR, PAIRS+1, TAG_INIT, MPI_COMM_WORLD);

	// Affiche les fingers de chaque pair
	affichage_fingers(pairs, fingers);
	
	MPI_Barrier(MPI_COMM_WORLD);
	// A patir d'ici, tous les pairs sont initialisés (MPI_barrier), on va pouvoir faire des requêtes

	/* FIN INITIALISATION DES PAIRS */
}


// Réalise une recherche de la clé @recherche.
// @initiateur est le rang mpi du processus ayant initialisé la recherche
void rechercher(int recherche, int initiateur)
{
	int cle = recherche%(SIZE);
	
	// Si la clé est dans l'intervalle ]this , successeur] alors c'est notre successeur qui la gère
	if(dans_intervalle_b_inclus(cle, this.chordid, fingers[0].chordid))
	{
		// envoi de la clé recherchée au successeur
		MPI_Send(&recherche, 1, MPI_INT, fingers[0].rang, TAG_RESPONSABLE, MPI_COMM_WORLD);

		// envoi de l'id du pair courrant pour savoir qui est l'initiateur
		MPI_Send(&initiateur, 1, MPI_INT, fingers[0].rang, TAG_RESPONSABLE, MPI_COMM_WORLD);

		printf("{%d,%d} a transféré la recherche au pair responsable ({%d,%d}) de %d.\n", this.rang, this.chordid, fingers[0].rang, fingers[0].chordid, recherche);
	}
	else // Sinon, on transmet à notre meilleur finger (celui qui est le plus proche ou le plus grand)
	{
		pair* finger_proche = NULL;
		int trouve = 0;
		for(int i=M-1;i>=0;i--)
		{
			// Si le finger est dans l'intervalle fermé ]this , cle[ alors on lui envoie la requête
			if(dans_intervalle_ferme(fingers[i].chordid,this.chordid, cle))
			{
				finger_proche=&fingers[i];
				trouve = 1;
				break;
			}
		}
		// Si on a pas trouvé on passe la requête à notre successeur
		if(!trouve) finger_proche=&fingers[0];

		// envoi de la clé au finger le plus proche de la valeur reçue
		MPI_Send(&recherche, 1, MPI_INT, finger_proche->rang, TAG_TRANSFERT, MPI_COMM_WORLD);

		// envoi de l'id du pair courrant pour savoir qui est l'initiateur
		MPI_Send(&initiateur, 1, MPI_INT, finger_proche->rang, TAG_TRANSFERT, MPI_COMM_WORLD);

		printf("{%d,%d} a transféré la recherche de %d à {%d,%d}.\n", this.rang, this.chordid, recherche, finger_proche->rang, finger_proche->chordid);
	}
}

void calculer_fingers(int source_rang)
{
	for(int i=0;i<M;i++) {
		int id = (this.chordid + (1 << i))%(1 << M);
		MPI_Send(&id, 1, MPI_INT, source_rang, TAG_RECHERCHE, MPI_COMM_WORLD);
		MPI_Recv(&fingers[i], 1, MPI_PAIR, MPI_ANY_SOURCE, TAG_RESULTAT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	char print[250] = "";
	for(int i=0;i<M;i++) sprintf(print,"%s{%d,%d} ", print, fingers[i].rang, fingers[i].chordid);
	printf("Fingers de {%d,%d} : %s\n", this.rang, this.chordid, print);
}

// Fonction d'exécution des pairs
void main_pair()
{
	MPI_Status status;
	// attend les informations du pair courrant (this)
	MPI_Recv(&this, 1, MPI_PAIR, INITIATEUR, TAG_INIT, MPI_COMM_WORLD, &status);
	// attend les fingers du pair courrant
	MPI_Recv(&fingers, M, MPI_PAIR, INITIATEUR, TAG_INIT, MPI_COMM_WORLD, &status);
	// attend les fingers du pair courrant
	MPI_Recv(&inverses, PAIRS+1, MPI_PAIR, INITIATEUR, TAG_INIT, MPI_COMM_WORLD, &status);

	// attend que tous les pairs soient initialisés
	MPI_Barrier(MPI_COMM_WORLD);

	int run = 1;
	while(run) {
		int valeur_recue; // valeur reçue dans les messages
		int initiateur_rang; // rang initiateur de la recherche

		MPI_Recv(&valeur_recue, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch(status.MPI_TAG) {
			// si on reçoit une demande de recherche
			case TAG_RECHERCHE:
				printf("{%d,%d} a reçu une recherche pour %d.\n", this.rang, this.chordid, valeur_recue);
				rechercher(valeur_recue, status.MPI_SOURCE);
				break;
			case TAG_TRANSFERT:
				MPI_Recv(&initiateur_rang, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				rechercher(valeur_recue, initiateur_rang);
				break;
			// on est responsable de la clé recherchée
			case TAG_RESPONSABLE:
				MPI_Recv(&initiateur_rang, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // pair ayant initié la recherche
				MPI_Send(&this, 1, MPI_PAIR, initiateur_rang, TAG_RESULTAT, MPI_COMM_WORLD);
				printf("{%d,%d} a envoyé sa réponse pour %d au processus ayant fait la requête (P%d).\n", this.rang, this.chordid, valeur_recue, initiateur_rang);
				break;
			case TAG_REJOINT:
				printf("eeeeeeeeeeeeeeeeee\n");
				MPI_Recv(&initiateur_rang, 1, MPI_INT, MPI_ANY_SOURCE, TAG_REJOINT, MPI_COMM_WORLD, &status);
				printf("fffffffffffffff\n");
				// Il faut prévenir les pairs qui référencent le pair courrant comme finger.
				for(int i=0;i<PAIRS+1;i++) {
					if(inverses[i].rang!=-1) {
						MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, inverses[i].rang, TAG_ACTUALISER, MPI_COMM_WORLD);
					}
				}
				break;
			case TAG_ACTUALISER:
				printf("GOOOOOOOOOOOOOOOOO\n");
				MPI_Recv(&initiateur_rang, 1, MPI_INT, MPI_ANY_SOURCE, TAG_ACTUALISER, MPI_COMM_WORLD, &status);
				
				
				calculer_fingers(initiateur_rang);
				break;
			case TAG_FIN:
				run = 0;
				break;
		}
	}
}

void main_rejoindre()
{
	MPI_Status status;
	// attend les informations du pair courrant (this)
	MPI_Recv(&this, 1, MPI_PAIR, MPI_ANY_SOURCE, TAG_INIT, MPI_COMM_WORLD, &status);
	// attend les identifiants du pair connu
	printf("Nouveau pair : {%d,%d}\n",this.rang, this.chordid);
	pair pair_connu;
	MPI_Recv(&pair_connu, 1, MPI_PAIR, INITIATEUR, TAG_INIT, MPI_COMM_WORLD, &status);

	// attend que tous les pairs soient initialisés
	MPI_Barrier(MPI_COMM_WORLD);
	
	sleep(1); // attend un peu histoire de...
	
	// calcule des fingers
	calculer_fingers(pair_connu.rang);

	// previent son successeur qu'il s'est inséré.
	MPI_Send(&this.rang, 1, MPI_INT, fingers[0].rang, TAG_REJOINT, MPI_COMM_WORLD);
	printf("____________%d_____________\n", fingers[0].rang);
}
/******************************************************************************/

int main(int argc, char* argv[])
{
	int nb_proc;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

	if (nb_proc != PAIRS+2) {
		printf("Nombre de processus incorrect, il en faut %d !\n", PAIRS+2);
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

	if (rang == 0) simulateur();
	else if(rang == RANG_REJOINDRE) main_rejoindre();
	else main_pair();

	MPI_Finalize();
	return 0;
}
