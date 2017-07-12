#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "../Headers/strtools.h"

typedef struct personne 
{
	char nom[50];
	int age;
	int salaire;
} personne;

void main()
{
  /*
	struct personne pers[3];
	int i=0,f,res;
	float m=0;
	for(i=0; i<3; i++)
	{
		printf("Donner le nom de personne %d: ",i);
		scanf("%s",&pers[i].nom);
		printf("Donner l'age de personne %d: ",i);
		scanf("%d",&pers[i].age);
		printf("Donner la salaire de personne %d: ",i);
		scanf("%d",&pers[i].salaire);
	}
	f = open("/home/ali/Desktop/res.bin","O_CREATE|O_WRONLY|O_TRUNC");
	if(f == -1)
	{
		fprintf(stderr,"Erreur D'ouverture de fichier!\n");
	}
	for(i=0; i<3; i++)
	{
		res = write(f,&pers[i],sizeof(personne));
		if(res==-1)
		{
			fprintf(stderr,"Erreur D'ecriture!");
		}
	}
	*/
  char event[100];
  printf("Event: ");
  fgets(event , 100 , stdin);
  event[strlen(event) - 1] = '\0';
  pLog(__func__ , event);
  pLog(__func__ , "Logging %s Complete\n" , event);
}
