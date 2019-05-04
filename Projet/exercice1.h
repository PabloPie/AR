#include <mpi.h>

#ifndef EXO1_H
#define EXO1_H

typedef struct pair
{
	int rang;
	int chordid;
} pair;

MPI_Datatype MPI_PAIR;

#define INITIATEUR				0
#define TAG_INIT				0
#define TAG_FIN					1
#define TAG_RECHERCHE			2
#define TAG_TRANSFERT			3
#define TAG_RESPONSABLE			4
#define TAG_RESULTAT			5
#define SIZE 1 << M

static int rang; // rang mpi

static int I[SIZE]; // I == K
static inline int f(int v) { // f(x) == g(x') car I == K
	return I[v%(SIZE)];
// simulation avec pairs du td :
/*	switch(v) {
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

inline static void init_ensemble_I()
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

static int compare_pair(void const *a, void const *b)
{
	pair* pa = (pair*)a;
	pair* pb = (pair*)b;
	return pa->chordid - pb->chordid;
}

// teste si k se trouve dans l'intervalle ]a,b[ == (a,b)
int dans_intervalle_ferme(int k, int a, int b)
{
	return (a<b) ? (k > a && k < b) : ((k > a && k < SIZE) || (k > 0 && k < b));
	/*if( a < b ) return k > a && k < b;
	else  return (k > a && k < SIZE) || (k > 0 && k < b);
	return 0;*/
}

// teste si k se trouve dans l'intervalle [a,b[ == [a,b)
/*int dans_intervalle_a_inclus(int k, int a, int b)
{
	return (a<b) ? (k >= a && k < b) : ((k >= a && k < SIZE) || (k >= 0 && k < b));
	/*if( a < b ) return k >= a && k < b;
	else return (k >= a && k < SIZE) || (k >= 0 && k < b);
	return 0;*/
//}

// teste si k se trouve dans l'intervalle ]a, b] == (a,b]
int dans_intervalle_b_inclus(int k, int a, int b)
{
	return (a<b) ? (k > a && k <= b) : ((k > a && k < SIZE) || (k >= 0 && k <= b));
	/*if( a < b ) return k > a && k <= b;
	else return (k > a && k < SIZE) || (k >= 0 && k <= b);
	return 0;*/
}


static inline void affichage_fingers(pair* pairs, pair fingers[PAIRS][M])
{
	printf("------------------------------------------------------------\n");
	for(int i=0;i<PAIRS;i++) {
		printf("Fingers de {%d;%d} :", pairs[i].rang, pairs[i].chordid);
		for(int j=0;j<M;j++) printf("\t{%d;%d}", fingers[i][j].rang, fingers[i][j].chordid);
		printf("\n");
	}
	printf("------------------------------------------------------------\n");
}


#endif
