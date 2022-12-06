#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <string.h>


/** ********************************************** **/
/** Definition des constantes de la Simulation     **/

#define exp_i (double)0.1
#define exp_e (double)10
#define exp_w (double)0.25
#define max_sent (int)1e3
#define vision (int)0 //affichage de l'echéancier -> 2 ; seulement les changement d'état -> 1 ; rien 0

/** ********************************************** **/
/** Structure pour les événements de la Simulation **/

typedef struct Event event;

struct Event{
    int c;			//numero du capteur qui doit emettre 
    int etat; 		//0 etat i, 1 emission et 2 attente
    int num_etat; 	//[1,7]
    double debut;
    double fin;
    event *suivant;	//Pointeur sur le prochain evenement
};

/** ********************************************** **/
/** Structure pour les capteurs de la Simulation   **/

typedef struct Capteur{
    int lost;					//paquets perdus
    int sent;					//paquets envoyés
    int etat;					//0 etat i, 1 emission et 2 attente
}capteur;

/** ********************************************** **/
/** Structure pour l'echeancier de la Simulation   **/

typedef struct Echeancier{
    event *premier;		//Premier élement stocké par l'échéancier
}echeancier;

/** ********************************************** **/
/** Definition des variables globales              **/

int K = 5; //nombre de capteur
double temps_total = 0; //Variable qui va recuperer le temps de la simulation a la fin. 
echeancier Ech;	//Premier élément de la liste chainée
capteur* c;	//Tableau des capteurs
int nb_collisisons[7];	//Nombre de collisions pour chaque état
int nb_emissions[7];	//Nombre de tentatives d'émission pour chaque état
double t_emission[7];	//temps d'émission pour chaque état

/** ********************************************** **/
/** Fonction qui simule une loi exponentielle      **/

double Expo_Duree(double lambda)
{	
	double r = 0;
	while(r == 0 ||r == 1 ) r = (double) rand()/RAND_MAX;	//Ici on exclu 0 et 1 car log(0) n'est pas défini et log(1) = 0 et on veut toujours un temps d'attente.
	return (double) -log(r) / lambda;
}

/** ********************************************************* **/
/** Fonction bit mix (https://stackoverflow.com/a/323302)     **/

unsigned long mix(unsigned long a, unsigned long b, unsigned long c)
{
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}

/** ********************************************** **/
/** Fonction d'ajout a l'echeancier                **/

void Ajouter_Ech(event e)
{	
	event* tmp = malloc(sizeof(event));				//On créé un nouvel event qui prend les valeurs de e
	tmp->c = e.c;
	tmp->debut = e.debut;
	tmp->fin = e.fin;
	tmp->etat = e.etat;
	tmp->num_etat = e.num_etat;
	tmp->suivant = NULL;
	
	if(Ech.premier == NULL) Ech.premier = tmp;		//Si echeancier vide tmp devient le premier element.
	else if(Ech.premier->fin >= tmp->fin) 
	{
		tmp->suivant = Ech.premier; 				//Sinon Si le premier element a une date > tmp, tmp devient le premier element et le premier le deuxieme
		Ech.premier = tmp;
	}
	else 											//Sinon												 	
	{
		event* navigation = Ech.premier->suivant;	//On se place sur le deuxieme element
		event* precedent = Ech.premier;				//On stocke le premier
		
		while(navigation != NULL)
		{
			if(navigation->fin >= tmp->fin)			//Si tmp a une date plus petite que navigation
			{	
				tmp->suivant = navigation;			//tmp pointe sur navigation
				navigation = NULL;					//On coupe la boucle while
			}
			else 
			{	
				precedent = navigation;				//Sinon precedent devient navigation
				navigation = navigation->suivant; 	//Et navigation devient suivant, on avance dans l'echeancier
			}
		}
		precedent->suivant = tmp;					//precedent pointe sur tmp, on a donc placé tmp entre precedent et navigation
	}
}


/** ********************************************** **/
/** Fonction d'initialisation de la Simulation     **/

void Initialisation() 
{	
	c = malloc(K*sizeof(capteur)); 
	
	temps_total = 0;
	
	Ech.premier = NULL; //Initialisation de l'echeancier
	
	for(int i = 0; i < 7; i++) //Initialisation des variables globales
	{
		nb_collisisons[i] = 0;
		nb_emissions[i] = 0;
		t_emission[i] = 0;
	}
	
    for(int i = 0; i < K; i++) 
    {
        c[i].etat = 0;	//Initialisation des capteurs
        c[i].lost = 0;
        c[i].sent = 0;

        event e;	//Remplissage de l'échéancier avec les premiers événements
        e.c = i;
        e.etat = 0;
        e.num_etat = 0;
        e.debut = 0;
        e.fin = (double) Expo_Duree(exp_i);
        Ajouter_Ech(e);
    }
}

/** ********************************************** **/
/** Fonction qui detecte les conditions d'arret    **/

int stop()
{	
	int cpt = 0;
	
	for(int i = 0; i < K; i++) if(c[i].sent >= max_sent) cpt++; //le compteur prend la valeur du nombres de capteur a max_sent paquets envoyé
	
	if(cpt == K) return 0;	//Si tous les capteurs on envoyé max sent on renvoie 0 (coupe la boucle)
	else return 1;			//Sinon on renvoi 1
}

/** ********************************************** **/
/** Fonction qui affiche l'echeancier (debogue)    **/

void Affiche_Ech()
{	
	int cpt = 1;
	event* navigation = Ech.premier;
	while(navigation != NULL)
	{	
		printf("n°%d - ",cpt);
		printf("c%d |",navigation->c+1);
		printf("t : %f |",navigation->fin);
		
		if(navigation->etat == 0) printf("i\n");
		else if(navigation->etat == 1)
		{
			printf("e");
			printf("%d\n",navigation->num_etat);
		}
		else 
		{
			printf("w");
			printf("%d\n",navigation->num_etat);	
		}
		
		cpt++; navigation = navigation->suivant;
	}
}

/** ********************************************** **/
/** Fonction de traitement des collisions          **/

void Traitement_Collision(int c1 , int c2)	//c1 et c2 les numero des capteurs en collision.
{											//Avec c1 le capteur qui emettait deja et c2 celui qui vient de passer en emission. 
	
	event* navigation = Ech.premier;	//On se place sur le premier element
	event* precedent = NULL;			//On defini precedent qui prendra l'element avant navigation
	event tmp;
	double temps;	//Variable qui va stocker le temps t ou la colision a eu lieu
	int bool = 1;	//Variable pour la boucle while
	
	while(bool)									//Traitement de la collision sur c2
	{	
		if(navigation->c == c2)					//Si navigation represente l'event du capteur c2
		{	
			bool = 0;							//On coupe la boucle while
		}
		else 
		{	
			precedent = navigation;				//Sinon precedent devient navigation
			navigation = navigation->suivant; 	//Et navigation devient suivant, on avance dans l'échéancier
		}
	}
	
	if(precedent == NULL) Ech.premier = Ech.premier->suivant; //Si navigation est le premier element, Ech.premier = Ech.suivant
	else precedent->suivant = navigation->suivant;			  //Sinon on precedent suivant = navigation suivant
	
	nb_collisisons[navigation->num_etat]++; //On augmente les collisions de l'état (pas de -1 car il passe de wj à ej+1)
	nb_emissions[navigation->num_etat]++;	//On incremente le nb de tentatives d'émissions de l'état
	
	temps = navigation->fin;	//on stock le temps ou la collision a eu lieu
			
	if(navigation->num_etat + 1 == 7)
	{
		if(vision) printf("Le capteur %d perd son paquet et passe en état initial\n",c2+1);
		
		c[c2].lost += 1;	//On dit qu'on a perdu un paquet
		c[c2].etat = 0;	//On met le capteur en état initial
								
		tmp.c = c2;		//On créé un nouvel event initial				
		tmp.etat = 0;
		tmp.num_etat = 0;
		tmp.debut = navigation->fin; //Son debut deviens le moment t de la collision
		tmp.fin = (double) navigation->fin + Expo_Duree(exp_i); //On ajoute le temps E(i)
	}
	else
	{
		c[c2].etat = 2;	//On met le capteur en attente		
							
		if(vision) printf("Le capteur %d passe attente\n",c2+1);
		
		tmp.c = c2;		//On créé un nouvel event en attente							
		tmp.etat = 2;
		tmp.num_etat = navigation->num_etat + 1; //+1 car ici on saute l'etat emission j+1 on passe direct en attente j+1
		tmp.debut = navigation->fin; 
		tmp.fin = (double) navigation->fin + Expo_Duree(exp_w); //on ajoute le temps d'attente E(w)
	}
	
	Ajouter_Ech(tmp); //On ajoute le nouvel event a l'echeancier	
	
	free(navigation);	//On free navigation
	
	navigation = Ech.premier;	//On reset navigation, precedent et bool pour traiter la deuxieme collision
	precedent = NULL;				
	bool = 1;
	
	while(bool)									//Traitement de la collision sur c1
	{	
		if(navigation->c == c1)					//Si navigation represente l'event du capteur c1
		{	
			bool = 0;							//On coupe la boucle while
		}
		else 
		{	
			precedent = navigation;				//Sinon precedent devient navigation
			navigation = navigation->suivant; 	//Et navigation devient suivant, on avance dans l'échéancier
		}
	}
	
	if(precedent == NULL) Ech.premier = Ech.premier->suivant;	//Si navigation est le premier element, Ech.premier = Ech.suivant
	else precedent->suivant = navigation->suivant;				//Sinon on precedent suivant = navigation suivant
	
	t_emission[navigation->num_etat-1] += (double) temps - navigation->debut; //calcul du temps d'emission avant collision
	
	nb_collisisons[navigation->num_etat-1]++; //On augmente les collisions de l'état
	nb_emissions[navigation->num_etat-1]++;	//On incremente le nb de tentatives d'émissions de l'état
	
	if(navigation->num_etat == 7) //Si l'event qui etait en emission etait en état e7
	{	
		if(vision) printf("Le capteur %d perd son paquet et passe en état initial\n\n",c1+1);
		
		c[c1].lost += 1;	//On dit qu'on a perdu un paquet
		c[c1].etat = 0;	//On met le capteur en état initial
								
		tmp.c = c1;		//On créé un nouvel event initial				
		tmp.etat = 0;
		tmp.num_etat = 0;
		tmp.debut = temps; //Son debut deviens le moment t de la collision
		tmp.fin = (double) temps + Expo_Duree(exp_i); //On ajoute le temps E(i)
	}
	else
	{	
		if(vision) printf("Le capteur %d passe attente\n\n",c1+1);
		
		c[c1].etat = 2;	//On met le capteur en attente
								
		tmp.c = c1;		//On créé un nouvel event en attente			
		tmp.etat = 2;
		tmp.num_etat = navigation->num_etat;
		tmp.debut = temps; 
		tmp.fin = (double) temps + Expo_Duree(exp_w);	//On ajoute le temps E(w)
	}

	Ajouter_Ech(tmp);	//On ajoute l'event a l'échéancier
	
	free(navigation);	//On free navigation
	
}

/** ********************************************** **/
/** Fonction de traitement des événements          **/

void Traitement_Event(FILE* fi,char *value)
{
	event* e = Ech.premier;	//recuperation de l'event a traiter (trier donc tj le premier)
	event tmp;
	
	if(vision) printf("\ntemps = %f\n",e->fin);
	
	int collision = 0;		//on regarde si un capteur emet deja
	
	for(int i = 0; i < K; i++) //on parcours tout les capteurs et on regarde si un emet
	{
		if(c[i].etat == 1) { collision = i+1; break; }	//si oui, collision prend le numero du capteur +1 en emission (+1 car on a 0) 
	}
	
	if(e->etat == 0 || e->etat == 2)				//Sinon si etat initial ou attente, on passe en emission
	{	
		if(collision) 								//Si un capteur est deja en emission, Collision
		{	
			if(vision) printf("Collision entre %d et %d\n",collision,e->c+1); 
			Traitement_Collision(collision-1,e->c);	//-1 car on avait +1 pour prendre le cas du 0
			
		}
		else 										//Sinon pas de collision on le traite normalement
		{	
			if(vision) printf("Le capteur %d passe en émission\n\n",e->c+1);
			c[e->c].etat = 1;						//Le capteur passe en emission
			
			tmp.c = e->c;							//On créé le nouvel event 
			tmp.etat = 1;
			tmp.num_etat = e->num_etat + 1;
			tmp.debut = e->fin; 
			tmp.fin = (double) e->fin + Expo_Duree(exp_e);
			Ech.premier = Ech.premier->suivant;		//On supprime l'evenement qui vient d'etre traité
			free(e);	//on free l'event qu'on vient de traiter
			if(tmp.num_etat == 1) if(strcmp(value,"f3")  == 0) fprintf(fi,"%f\n",tmp.fin - tmp.debut);
			if(tmp.num_etat == 2) if(strcmp(value,"f4")  == 0) fprintf(fi,"%f\n",tmp.fin - tmp.debut);
			Ajouter_Ech(tmp);						//On ajoute le nouvel event
		}
		if(strcmp(value,"f1")  == 0) fprintf(fi,"%f  ",e->fin);
		
	}
	else if(e->etat == 1)							//Sinon si etat emssion, on passe en état initial
	{	
		if(vision) printf("Le capteur %d passe en état initial\n\n",e->c+1);
		c[e->c].etat = 0;	//On met le capteur en etat initial
        c[e->c].sent++;	//On indique que le paquet a bien été envoyé
        
        t_emission[e->num_etat-1] += (double) e->fin - e->debut; //-1 car num_etat de 1à7 et t_emission de 0à6
																 //Le temps d'emission correspond au temps de fin moins le debut
		nb_emissions[e->num_etat-1]++;	//On incremente le nb de tentatives d'émissions de l'état
		tmp.c = e->c;	//on créé un nouvel event 
        tmp.etat = 0;
        tmp.num_etat = 0;
        tmp.debut = e->fin; 
		tmp.fin = (double) e->fin + Expo_Duree(exp_i); //On ajoute le temps E(i)
		if(strcmp(value,"f1")  == 0) fprintf(fi,"%f  ",e->fin);
		Ech.premier = Ech.premier->suivant;	//on supprime l'event qu'on vient de traiter
		free(e);	//on free l'event qu'on vient de traiter
			
        Ajouter_Ech(tmp);	//On l'ajoute
		if(!stop()) temps_total = e->fin; //Ici on test si les capteurs on emis max_sent. Si oui temps_total prend le temps de fin du dernier evenement. Cela arrete automatiquement la simulation.
	}
	
		
}

/** ********************************************** **/
/** Fonction de fonctionnement de la Simulation    **/

void Simulation(FILE* fi,char *value)
{
	Initialisation();
	printf("Debut de la simulation :\n\n");

	if(vision == 2) Affiche_Ech();
	
	while(!temps_total) //Si le temps total est toujours 0 c'est que la simulation n'est pas terminé (voir l.381)
	{	
		if(vision == 2) printf("~~~~~~~~~~~~~~~~~~~~~~\n");
		if(vision == 2) Affiche_Ech();
		Traitement_Event(fi,value);	
		for(int i=0;i<7;i++){
			double pourcentage = (double)nb_collisisons[i]/nb_emissions[i]*100;
			if(nb_emissions[i]){
				if(strcmp(value,"f1")  == 0) fprintf(fi,"  %f",pourcentage);
			} 
			else{
				if(strcmp(value,"f1")  == 0) fprintf(fi,"  0.0000  ");
			}
		}
		if(strcmp(value,"f1")  == 0) fprintf(fi,"  \n");
		if(vision == 2) Affiche_Ech();
		if(vision == 2) printf("~~~~~~~~~~~~~~~~~~~~~~\n");
	}
	
	if(vision) printf("\n");
	for(int i = 0 ; i < K; i++) printf("Le capteur %d à envoyé %d paquets\n",i+1,c[i].sent);
	printf("\n");
	for(int i = 0 ; i < K; i++) printf("Le capteur %d à perdu %d paquets\n",i+1,c[i].lost);
	printf("\n");
	for(int i = 0 ; i < 7; i++) printf("L'etat e%d à eu %d collisions\n",i+1,nb_collisisons[i]);
	printf("\n");
	for(int i = 0 ; i < 7; i++) printf("L'etat e%d a essayé d'émettre %d paquets\n",i+1,nb_emissions[i]);
	printf("\n");
	for(int i = 0 ; i < 7; i++)	printf("L'etat e%d à passé %f temps en émission\n",i+1,t_emission[i]);
	printf("\n");
	for(int i = 0 ; i < 7; i++) 
	{

		double pourcentage = (double)nb_collisisons[i]/nb_emissions[i]*100;
		
		if(nb_emissions[i]){
			printf("L'etat e%d à une proba de collision de %07.4f%%\n",i+1, pourcentage);
		} 
		else{
			printf("L'etat e%d à une proba de collision de 00.0000%%\n",i+1);
		}
			

	}
	if(strcmp(value,"f2")  == 0) fprintf(fi,"  %f\n",(double)nb_collisisons[1]/nb_emissions[1]*100);
	
	int col_total,emi_total;
	col_total=emi_total=0;
	
	for(int i = 0 ; i < 7 ; i++)
	{
		col_total += nb_collisisons[i];
		emi_total += nb_emissions[i];
	}
	double pourcentage_globlal = (double)col_total/emi_total*100;
	printf("\nProbabilité de collision global %07.4f%%\n",pourcentage_globlal);
	if(strcmp(value,"f5")  == 0) fprintf(fi," %f\n",pourcentage_globlal);
	printf("valeur de K%d\n",K);
	event* e = Ech.premier; //On vide l'échéancier
	event* precedent = NULL;
	
	while(e != NULL)
	{
		precedent = e;
		e = e->suivant;
		free(precedent);
	}
	
	free(c);
	
	printf("\nTemps total de la simulation : %f\n",temps_total);
	
	printf("\nFin de la simulation.\n\n");
}

/** ********************************************** **/
/** Main								           **/

int main(int argc, char const *argv[])
{	
	printf("----------------------------------\n");

	FILE *f1 = fopen("CourbePourcentageCollision.data","a");
	FILE *f2 = fopen("Courbe90Pourcen.data","a");
	FILE *f3 = fopen("HistogrameTempsE1.data","a");
	FILE *f4 = fopen("HistogrameTempsE2.data","a");
	FILE *f5 = fopen("CourbesMoyenneCollision.data","a");

	
	

	unsigned long seed = mix(clock(), time(NULL), getpid());
	printf("\nGRAINE : %lu\n\n",seed);

	srand(seed);
	if(argc == 2){
		char *tempo = argv[1];
		if(strcmp(argv[1],"f1")  == 0)Simulation(f1,tempo);
		else if(strcmp(argv[1],"f2") == 0)Simulation(f2,tempo);
		else if(strcmp(argv[1],"f3") == 0)Simulation(f3,tempo);
		else if(strcmp(argv[1],"f4") == 0)Simulation(f4,tempo);
		else if(strcmp(argv[1],"f5") == 0){
			
			for(K = 1;K<=100;K++){
				fprintf(f5,"  %d",K);
				Simulation(f5,tempo);
			}
		}
	}
	
	
	fclose(f1);
	fclose(f2);
	fclose(f3);
	fclose(f4);
	fclose(f5);
	// printf("----------------------------------\n");
	
	return 0;
}



