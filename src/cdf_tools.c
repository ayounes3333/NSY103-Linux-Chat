#include <stdio.h>
#include "../Headers/clients.h"

//print MSQ
int Print_MSQ(struct MSQ *msq)
{
  struct MSQ *t;
  if(msq == NULL)
  {
    fprintf(stderr , "msq NULL!\n");
  }
  printf("\n%s: %s\n" , msq->mes.src , msq->mes.message);
  if(msq->s == NULL)
    return 0;
  for(t = msq->s; t->s != NULL; t = t->s)
  {
    printf("\n%s: %s\n" , t->mes.src , t->mes.message);
  }
  return 0;
}
int Add_CLI()
{
  char *mes, src[20];
  struct Client *cli;
  struct MSQ *msq;
  struct lst_clients *l , *t;
  int i , c;
  cli = (Client *)malloc(sizeof(Client));
  msq = (MSQ *)malloc(sizeof(MSQ));
  mes = (char *)malloc(256);
  msq->s = NULL;
  printf("Name: ");
  scanf("%s" , cli->name);
  printf("Pass: ");
  scanf("%s" , cli->pass);
  printf("Message Count: ");
  scanf("%d" , &c);
  for(i = 0; i<c; i++)
  {
    printf("Message: ");
    scanf("\n");
    mes = fgets(mes , 256 , stdin);
    mes[strlen(mes) - 1] = '\0';
    printf("Source: ");
    scanf("%s" , src);
    msq = insert_msg(msq , mes , src);
  }
  cli->msq = msq;
  Print_MSQ(msq);
  printf("Saving %s....\n" , cli->name);
  Save_Client(cli);
}
int Show_CLIs()
{
  struct lst_clients *l , *t;
  l = Load_CDFs("./Clients");
  print_lst_cli(l);
  if(l != NULL)
  {
    printf("Success :)\n");
    for(t = l; t->s != NULL; t = t->s)
    {
      printf("\ntesting %s....\n" , t->c->name);
      Print_MSQ(t->c->msq);
    }
    return 0;
  } else {
    fprintf(stderr , "Error retreaving saved cdf!!\n");
    return -1;
  }
}
int main()
{
  char ch;
  while(1)
  {
    printf("\n\n##########   CDF TOOLS   ##########\n\n");
    printf("*1\tAdd Client.\n");
    printf("*2\tShow Clients.\n");
    printf("*0\tExit.\n\n");
    printf("\tChoice: ");
    ch = getchar();
    switch(ch)
    {
      case '1' :
      {
	Add_CLI();
      }
      break;
      case '2' :
      {
	Show_CLIs();      
      }
      break;
      case '0' :
      {
	exit(0);
      }
      break;
      default : 
	printf("Invalid Choice %c:%d!\n" , ch , ch);   
    }
  }  
}