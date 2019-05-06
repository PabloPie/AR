#include <mpi.h>

#ifndef EXO1_H
#define EXO1_H

typedef struct pair
{
	int rang;
	int chordid;
} pair;
MPI_Datatype MPI_PAIR;

typedef struct sinclair_round
{
	int initiateur;
	int distance;
} sinclair_round;
MPI_Datatype MPI_SINCLAIR;

#define TAG_INIT				0
#define TAG_ELECTION			1
#define TAG_LISTPAIRS_BUILD		3
#define TAG_LISTPAIRS_COMPLETE	4
#define SIZE 1 << M

// ensemble I
static int I[SIZE]; // I == K

// fonction de hachage des clés
static inline int f(int v) { // f(x) == g(x') car I == K
	return I[v%(SIZE)];
// simulation avec pairs du td :
	/*switch(v) {
		case 0 : return 2; break;
		case 1: return 7; break;
		case 2: return 13; break;
		case 3: return 14; break;
		case 4: return 21; break;
		case 5: return 38; break;
		case 6: return 42; break;
		case 7: return 48; break;
		case 8: return 51; break;
		case 9: return 59; break;
		default: return I[v%(SIZE)];
	}*/
}

// Tirage aléatoire de l'ensemble I avec unicité
static inline void init_ensemble_I()
{
	printf("Initialisation de l'ensemble I...\n");
	int end,value;
	for(int i=0;i<SIZE;i++) {
		while(1) {
			end = 1;
			value = rand()%(SIZE);
			for(int j=0;j<i;j++) {
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

// pour la fonction qsort
static int compare_pair(void const *a, void const *b)
{
	pair* pa = (pair*)a;
	pair* pb = (pair*)b;
	return pa->chordid - pb->chordid;
}

// teste si k se trouve dans l'intervalle ]a,b[ == (a,b)
static inline int dans_intervalle_ferme(int k, int a, int b)
{
	return (a<b) ? (k > a && k < b) : ((k > a && k < SIZE) || (k > 0 && k < b));
}

// teste si k se trouve dans l'intervalle ]a, b] == (a,b]
static inline int dans_intervalle_b_inclus(int k, int a, int b)
{
	return (a<b) ? (k > a && k <= b) : ((k > a && k < SIZE) || (k >= 0 && k <= b));
}



#endif
