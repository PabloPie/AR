#include <stdio.h>
#include <mpi.h>
#include <unistd.h>

//************   LES TAGS
#define WANNA_CHOPSTICK 0		// Demande de baguette
#define CHOPSTICK_YOURS 1		// Cession de baguette
#define DONE_EATING     2		// Annonce de terminaison

//************   LES ETATS D'UN NOEUD
#define THINKING 0   // N'a pas consomme tous ses repas & n'attend pas de baguette
#define HUNGRY   1   // En attente de baguette
#define DONE     2   // A consomme tous ses repas

//************   LES REPAS
#define NB_MEALS 3	// nb total de repas a consommer par noeud

//************   LES VARIABLES MPI
int NB;               // nb total de processus
int rank;             // mon identifiant
int left, right;      // les identifiants de mes voisins gauche et droit

//************   LA TEMPORISATION
static int local_clock = 0;                    // horloge locale
static int clock_val;                          // valeur d'horloge recue
static int meal_times[NB_MEALS];        // dates de mes repas

//************   LES ETATS LOCAUX
int local_state = THINKING;		// moi
int left_state  = THINKING;		// mon voisin de gauche
int right_state = THINKING;		// mon voisin de droite

//************   LES BAGUETTES 
int left_chopstick = 0;		// je n'ai pas celle de gauche
int right_chopstick = 0;	// je n'ai pas celle de droite

//************   LES REPAS 
int meals_eaten = 0;		// nb de repas consommes localement


//************   LES FONCTIONS   *************************** 
int max(int a, int b) {
   return (a>b?a:b);
}

void receive_message(MPI_Status *status) {
	MPI_Recv(&clock_val, sizeof(int), MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);
	local_clock = max(local_clock + 1, clock_val + 1);
}

void send_message(int dest, int tag) {
	MPI_Send(&local_clock,sizeof(int),MPI_INT,dest,tag,MPI_COMM_WORLD);
}

/* renvoie 0 si le noeud local et ses 2 voisins sont termines */
int check_termination() {
	return local_state!=DONE || left_state!=DONE || right_state!=DONE;
}

//************   LE PROGRAMME   *************************** 
int main(int argc, char* argv[]) {
	MPI_Status status;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &NB);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
   left = (rank + NB - 1) % NB;
   right = (rank + 1) % NB;

	while(check_termination())
	{
		/* Tant qu'on n'a pas fini tous ses repas, redemander les 2 baguettes  
		a chaque fin de repas */
		if ((meals_eaten < NB_MEALS) && (local_state == THINKING))
		{
			//demande de baguette aux 2 voisins
			local_state = HUNGRY;
			send_message(left, WANNA_CHOPSTICK);
			send_message(right, WANNA_CHOPSTICK);

		}
		printf("P%d [time=%d]> atttend message...\n",rank, local_clock);
		receive_message(&status);

		if (status.MPI_TAG == DONE_EATING)
		{
			printf("P%d [time=%d]> %d a terminé de manger\n", rank, local_clock, status.MPI_SOURCE);
			if(status.MPI_SOURCE == left) left_state = DONE;
			else if(status.MPI_SOURCE == right) right_state = DONE;
		}
		else if (status.MPI_TAG == WANNA_CHOPSTICK)
		{
			printf("P%d [time=%d]> reçoit une demande de baguette de %d\n", rank, local_clock, status.MPI_SOURCE);
			
			if(status.MPI_SOURCE == left) left_state = HUNGRY;
			else right_state = HUNGRY;
			
			if(local_state == HUNGRY)
			{
				printf("P%d [time=%d]> et %d veulent la même baguette\n", rank, local_clock, status.MPI_SOURCE);
				if(rank > status.MPI_SOURCE) // Je ne suis pas prio donc je donne et je redemande
				{
					if(status.MPI_SOURCE == left) left_chopstick = 0;
					else right_chopstick = 0;
					send_message(status.MPI_SOURCE, CHOPSTICK_YOURS);
					send_message(status.MPI_SOURCE, WANNA_CHOPSTICK);
				}
			}
			else send_message(status.MPI_SOURCE, CHOPSTICK_YOURS);
		}
		else if (status.MPI_TAG == CHOPSTICK_YOURS)
		{
			printf("P%d [time=%d]> reçoit une baguette de %d\n", rank, local_clock, status.MPI_SOURCE);
			
			if(status.MPI_SOURCE == left) left_chopstick = 1;
			else if(status.MPI_SOURCE == right) right_chopstick = 1;
			
			if(left_chopstick && right_chopstick && local_state == HUNGRY)
			{
				printf("P%d [time=%d]> mange pour la %d fois\n",rank, local_clock, meals_eaten);
				left_chopstick=0;
				right_chopstick=0;
				meal_times[meals_eaten] = local_clock;

				meals_eaten++;

				if(meals_eaten >= NB_MEALS)
				{
					local_state = DONE;
					printf("P%d [time=%d]> a mangé tous ses repas.\n",rank, local_clock);
					send_message(left, DONE_EATING);
					send_message(right, DONE_EATING);
				}
				else local_state = THINKING;

				if(left_state == HUNGRY) send_message(left, CHOPSTICK_YOURS);
				if(right_state == HUNGRY) send_message(right, CHOPSTICK_YOURS);
				
			}
		}
		
		local_clock++;
	}
	
	int r=0;
	char buf[75];
	sprintf(buf, "P%d date des repas : ",rank);
	for(;r<NB_MEALS;r++) sprintf(buf, "%s, %d", buf, meal_times[r]);
	printf("%s\n",buf);

	
   MPI_Finalize();
   return 0;
}
