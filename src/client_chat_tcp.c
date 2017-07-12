#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "../Headers/clients.h"

//client ID
char *id;

//SIGCHLD Handler
void chldexit()
{
  printf("Closing...\n");
  union wait wstat;
  pid_t	pid;
  pid = wait3(&wstat, WNOHANG, (struct rusage *)NULL );
  if(pid > 0)
  {
    pLog(__func__ , "Return code: %d", wstat.w_retcode);
    exit(0);
  }
  exit(-1);
}

//Get the client status
char *Get_Cli_Stat(int soc , char *name)
{
  struct Packet *p;
  char *stat;
  p = (Packet *)malloc(sizeof(Packet));
  Send_Packet(soc , id , name , "SERVER" , 3);
  p = Recive_Packet(soc);
  if(p == NULL)
  {
    pLog(__func__ , "Error Reciving Client Status");
    return NULL;
  }
  if(p->type == 3)
  {
    stat = (char *)malloc(strlen(p->message));
    sprintf(stat , "%s" , p->message);
    return stat;
  } else {
    fprintf(stderr , "Expected type 3 Packet but recived %d!\n" , p->type);
    pLog(__func__ , "Expected type 3 Packet but recived %d!" , p->type);
    return NULL;
  }
}

//Select Destination
char *Select_Dest(int soc , char **clis , int c)
{
  int i , ch;
  system("clear");
  printf("\n\n\n########################################\n\n");
  printf("\t\t\t\t\t\tSelect Client\n\n");
  printf("########################################\n\n");
  printf("\t\tSelect One of the folowing Clients:\n");
  for(i = 0; i<c; i++)
  {
    printf("\t*%d\t%s\n" , i , clis[i]);
  }
  do
  {
    printf("Choice: ");
    scanf("%d" , &ch);
    if(ch >= 0 && ch < c)
    {
      
      printf("%s is %s\n" , clis[ch] , Get_Cli_Stat(soc , clis[ch]));
      return clis[ch];
    }
    fprintf(stderr , "Invalid Choice\n");
  } while(ch < 0 || ch >= c);
}

int main(int argc , char *argv[])
{
  int st;
  int type;
  int proc;
  int servport;
  int s;
  int cli_count;
  char **clis;
  struct sockaddr_in serveur;
  socklen_t lgserveur;
  char *adrserveur;
  char *mes;
  char *pass;
  char *dest;
  char *c;
  struct Packet *rep;
  
  id = (char *)malloc(20);
  adrserveur = (char *)malloc(32);
  mes = (char *)malloc(256);
  dest = (char *)malloc(20);
  pass = (char *)malloc(300);
  rep = (Packet *)malloc(sizeof(Packet));
  signal (SIGCHLD, chldexit);
  printf("Starting TCP Chat Client...\n");
  if(argc != 3)
  {
    fprintf(stderr , "Usage: %s <IP> <Port> \n",argv[0]);
    pLog(__func__ , "Invalid Arguments Detected!");
    return -1;
  }
  servport = atoi(argv[2]);
  pLog(__func__ , "Port: %d",servport);
  if(servport <=0 || servport > 65535 )
  {
    fprintf(stderr , "Invalid Port!\n");
    pLog(__func__ , "Invalid Port!");
    return -1;
  }
  adrserveur = argv[1];
  if(inet_aton(adrserveur,&serveur.sin_addr) == 0)
  {
    fprintf(stderr , "Invalid IP Address!\n");
    pLog(__func__ , "Invalid IP Address!");
    return -1;
  }
  printf("Creating socket...\t\t");
  s = socket(AF_INET,SOCK_STREAM,6);
  if(s == -1)
  {
    printf("[ err ]\n");
    pLog(__func__ , "Socket Error!");
    return -1;
  }
  serveur.sin_family = AF_INET;
  serveur.sin_port = htons(servport);
  printf(" [ ok ]\n");
  printf("Estableshing Connection...\t\t");
  pLog(__func__ , "%s:%d" , argv[1] , ntohs(serveur.sin_port));
  st = connect(s,(const struct sockaddr *)&serveur,sizeof(serveur));
  if(st!=0)
  {
    printf("[ err ]\n");
    pLog(__func__ , "Connection refused! st = %d\n",st);
    return(-1);
  } pLog(__func__ , "connection established, Sending Chat ID.....");
  printf(" [ ok ]\n");
  printf("Username: ");
  scanf("%s",id);
  pass = getpass("Password: ");
  Send_Packet(s , id , "LOGIN_REQ" , "SERVER" ,  1);
  sprintf(mes , "%s&%s" , id , pass);
  sleep(1);
  Send_Packet(s , id , mes , "SERVER" , 1);
  rep = Recive_Packet(s);
  if(rep == NULL)
  {
    pLog(__func__ , "Authentication interrupted: Connection Lost!");
    return -1;
  }
  if(strcmp(rep->message , "LOGIN_ACK") == 0)
  {
    printf("Login Successful\n");    
  } else if(strcmp(rep->message , "LOGIN_REJ") == 0) 
  {
    fprintf(stderr , "Authentication failed\n");
    pLog(__func__ , "Authentication failed!");
    return -1;
  }
  printf("Destination: ");
  scanf("%s" , dest);
  c = Get_Cli_Stat(s , dest);
  if(c == NULL)
  {
    pLog(__func__ , "Get_Cli_Stat() Returned NULL\n");
    return -1;
  }
  printf("Client %s is %s\n" , dest , c);
  proc = fork();
  if(proc == 0) 
  {    
    do {
      printf("\nMessage: ");
      scanf("\n");
      mes = fgets(mes , 256 , stdin);
      mes[strlen(mes) - 1] = '\0';
      mes[strlen(mes)] = 0;
      Send_Packet(s , id , mes , dest , 0);
    } while(strcmp(mes,"fin")!=0);   
    st = Send_Packet(s , id , "DISCONNECT" , "SERVER" , 1);
    rep = Recive_Packet(s);
    if(rep == NULL)
    {
      fprintf(stderr , "Error Reiving DSC_ACK\n");
      pLog(__func__ , "Error Reciving DSC_ACK");
      exit(-1);
    }
    if(strcmp(rep->message , "DSC_ACK") != 0)
    {
      fprintf(stderr , "Exit Without DSC_ACK\n");
      pLog(__func__ , "Expected DSC_ACK!");	
      exit(0);
    }    
    exit(0);
  }
  else if(proc > 0)
  {
    do
    {
      rep = Recive_Packet(s);
      
      if(rep == NULL)
      {
	fprintf(stderr , "Packet recive error!\n");
	pLog(__func__ , "Packet Recive Error");
	kill(proc , SIGINT);
	wait;
	return -1;
      }
      else if(strcmp(rep->message , "UPDATE") == 0)
      {
	c = Get_Cli_Stat(s , dest);
	if(c == NULL)
	{
	  kill(proc , SIGINT);
	  wait;
	  exit(-1);
	}
	printf("%s is %s\n" , dest , c);
      }else if(strcmp(rep->message , "DSC_ACK") == 0) {
	printf("Closing...\n");
	kill(proc , SIGINT);
	wait;
	return 0;
      } else{
	printf("%c[2K", 27);
	printf("\t%s: %s\n" , rep->source , rep->message);
      }
    } while(strcmp(rep->message , "fin") != 0);
    kill(proc , SIGINT);
    wait;
    return 0;    
  }
}
