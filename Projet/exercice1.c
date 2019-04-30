#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define M		6
#define PAIRS	10


typedef struct pair {
	int rang;
	int chordid;
} pair;

#define INITIATEUR	0
#define TAGINIT		0
#define TAGFIN		1
#define TAGRECHERCHE	2
#define SIZE 1 << M
static pair fingers[M];
static pair successeur;
static pair id;
static int rang;

static int I[SIZE]; // I == K
static inline int f(int v) { // f(x) == g(x') car I == K
	//return I[v%(SIZE)];
	switch(v) {
		case 0 :
			return 2;
		break;
		case 1:
			return 7;
		break;
		case 2: return 13; break;
		case 3: return 14; break;
		case 4: return 21; break;
		case 5: return 38; break;
		case 6: return 42; break;
		case 7: return 48; break;
		case 8: return 51; break;
		case 9: return 59; break;
		default: return -1;
	}
}

inline static void init_ensemble_I() {
	printf("Initialisation de l'ensemble I...\n");
	int i,j,end,value;
	for(i=0;i<SIZE;i++) {
		while(1) {
			end = 1;
			value = rand()%(SIZE);
			for(j=0;j<i;j++) {
				if(value == I[j]) {
					end = 0;
					break;
				}
			}
			if(end) break;
		}
		I[i] = value;
	}
	printf("Ensemble I initialisé !\n");
}

static int compare_pair(void const *a, void const *b)
{
	pair* pa = (pair*)a;
	pair* pb = (pair*)b;
	return pa->chordid - pb->chordid;
}
	
void simulateur(void) {
	
	//srand(time(NULL));
	init_ensemble_I();
	
	int i;
	
	pair pairs[PAIRS];			// Liste des pairs
	pair fingers[PAIRS][M];		// Liste des fingers de chaque pair
	
	// Tirage aléatoire des id des pairs
	for(i=0;i<PAIRS;i++) pairs[i].chordid = f(i);
	qsort(&pairs, PAIRS, sizeof(pair), &compare_pair); // tri par ordre croissant
	for(i=0;i<PAIRS;i++) pairs[i].rang = i+1; // le rang mpi sans 0

	printf("IDs des pairs : ");
	for(i=0;i<PAIRS;i++) printf("{%d;%d} ", pairs[i].rang, pairs[i].chordid);
	printf("\n");

	// Initialisation des fingers
	for(i=0;i<PAIRS*M;i++) {
		fingers[i/M][i%M].rang = -1;
		fingers[i/M][i%M].chordid = -1;
	}
	// Calcul des fingers

	for(i=0;i<PAIRS;i++) { // Pour chaque pair
		printf("Fingers de {%d;%d} :", pairs[i].rang, pairs[i].chordid);

		// Pour chaque finger de pair[i]
		int j;
		for(j=0;j<M;j++) { 
			int inf = (pairs[i].chordid + (1 << j))%(1 << M);
			pair* pair_proche = &pairs[0];
			int k=0;
			// recherche du pair le plus proche de la valeur de inf
			while(k++<PAIRS && pair_proche->chordid < inf) pair_proche++;
			// pair_proche pointe vers le finger
			fingers[i][j] = *pair_proche;

			printf("\t{%d;%d}", fingers[i][j].rang, fingers[i][j].chordid);
		}
		printf("\n");
	}
					   
	for(i=0; i<PAIRS; i++){
		// envoi de l'id local
		MPI_Send(&pairs[i], 2, MPI_INT, pairs[i].rang, TAGINIT, MPI_COMM_WORLD);
		// envoi de l'id du successeur
		MPI_Send(&fingers[i][0], 2, MPI_INT, pairs[i].rang, TAGINIT, MPI_COMM_WORLD);
		// envoi des fingers
		MPI_Send(&fingers[i], 2*M, MPI_INT, pairs[i].rang, TAGINIT, MPI_COMM_WORLD);
	}
	
	sleep(5);
	for(i=0; i<PAIRS; i++) MPI_Send(NULL, 0, MPI_INT, pairs[i].rang, TAGFIN, MPI_COMM_WORLD);
}

void main_pair()
{
	MPI_Status status;
	MPI_Recv(&id, 2, MPI_INT, INITIATEUR, TAGINIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&successeur, 2, MPI_INT, INITIATEUR, TAGINIT, MPI_COMM_WORLD, &status);
	MPI_Recv(&fingers, 2*M, MPI_INT, INITIATEUR, TAGINIT, MPI_COMM_WORLD, &status);
	printf("{%d;%d} est initialisé : successeur {%d;%d} et fingers reçus\n", id.rang, id.chordid, successeur.rang, successeur.chordid);
	while(1) {
		int valeur;
		MPI_Recv(&valeur, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		
		if(status.MPI_TAG == TAGRECHERCHE) {
			
		}
		else if(status.MPI_TAG == TAGFIN) break;
	}
}

/******************************************************************************/

int main (int argc, char* argv[]) {
	int nb_proc;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

	if (nb_proc != PAIRS+1) {
		printf("Nombre de processus incorrect, il en faut %d !\n", PAIRS+1);
		MPI_Finalize();
		return 0;
	}

	MPI_Comm_rank(MPI_COMM_WORLD, &rang);

	if (rang == 0) simulateur();
	else main_pair();

	MPI_Finalize();
	return 0;
}
