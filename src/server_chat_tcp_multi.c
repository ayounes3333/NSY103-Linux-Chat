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
#include <errno.h>
#include <pthread.h>
#include "../Headers/clients.h"

//Clients count
int count_cli = -1;
//List of registred Clients
struct lst_clients *l;
//List of connected clients
struct lst_clients *Ol;

//Route a Message to its destination
int Route(Packet *p)
{
  char *mes;
  int st;
  struct Client *dest;
  dest = Get_cli(Ol , p->destination);
  if(dest != NULL)
  {
    pLog(__func__ , "Dest: %s@%d" , dest->name , dest->sock);
    st = Send_Packet(dest->sock , p->source , p->message , p->destination , 0);
    if(st==-1)
    {
      fprintf(stderr , "Error while Routing Packet!\n");
      pLog(__func__ , "Error while Routing Packet!");
      return(-1);
    } else return 0;
  } else {
    dest = Get_cli(l , p->destination);
    if(dest == NULL)
    {
      fprintf(stderr , "Client %s NOT FOUND\n" , p->destination);
      pLog(__func__ , "Client %s NOT FOUND" , p->destination);
    }
    dest->msq = insert_msg(dest->msq , p->message , p->source);
    Save_Client(dest);
    pLog(__func__ , "Client %s OFFLINE!" , p->destination);
  }
}

//Send an update signal to all online clients
int update(char *who)
{
  int st;
  struct lst_clients *t;
  for(t = Ol; t->s != NULL; t=t->s)
  {
    if(strcmp(t->c->name , who) == 0)
    {
      pLog(__func__ , "%s Skipped" , t->c->name);
      break;
    }
    st = Send_Packet(t->c->sock , "SERVER" , "UPDATE" , t->c->name , 3);
    if(st == -1)
    {
      fprintf(stderr , "Update Error\n");
      pLog(__func__ , "Update Error!");
      return -1;
    }
  }
  pLog(__func__ , "Update Finished");
  return 0;
}

//Test if a given port is free or in use
int isfreeport(int port)
{  
  struct servent *s;
  s = getservbyport(htons(port),NULL);
  if(s == NULL)
    return 1;
  else 
    return 0;
}

//Manage Session related (type 1) Packets
int SessionMGR(int soc , struct Packet *p)
{
  int st;
  struct Packet *rep;
  struct Client *c;
  char **ar;
  
  c = (Client *)malloc(sizeof(Client));
  rep = (Packet *)malloc(sizeof(Packet));
  if(strcmp(p->message , "LOGIN_REQ") == 0)
  {
    rep = Recive_Packet(soc);
    if(rep == NULL)
    {
      pLog(__func__ , "Error Reciving Credentials!");
      return -1;
    }
    ar = str_split(rep->message , "&" , 2);
    c = Get_cli(l , ar[0]);
    if(c == NULL)
    {
      Send_Packet(soc , "SERVER" , "LOGIN_REJ" , p->source , 1);
      fprintf(stderr , "Client %s NOT FOUND\n" , ar[0]);
      pLog(__func__ , "Client %s NOT FOUND" , ar[0]);
    }
    else if(strcmp(ar[1] , c->pass) != 0)
    {
      Send_Packet(soc , "SERVER" , "LOGIN_REJ" , p->source , 1);
      fprintf(stderr , "PASSWORD INCORRECT\n");
      pLog(__func__ , "Authentication Failed: PASSWORD INCORRECT!");
    } else {
      st = Send_Packet(soc , "Server" , "LOGIN_ACK" , p->source , 1);
      if(st > 0)
      {
	return 0;
      } else {
	fprintf(stderr , "Error sending LOGIN_ACK\n");
	pLog(__func__ , "Error Sending LOGIN_ACK!");
	return -1;
      }
    }
  }
  if(strcmp(p->message , "DISCONNECT") == 0)
  {
    st = Send_Packet(soc , "Server" , "DSC_ACK" , p->source , 1);
    if(st > 0)
    {
      pLog(__func__ , "Session Closed.");
      return 9;
    } else {
      fprintf(stderr , "Error sending DSC_ACK\n");
      pLog(__func__ , "Error sending DSC_ACK!");
      return -1;
    }
  }
  return 0;
}

//Manage Client requests
int ClientsMGR(int soc , struct Packet *p)
{
  int st;
  struct Client *c;
  c = Get_cli(l , p->message);
  if(c == NULL)
  {
    Send_Packet(soc , "SERVER" , "NOT FOUND" , p->source , 3);
    fprintf(stderr , "Error: client %s not found\n",p->message);
    pLog(__func__ , "Error: client %s not found!",p->message);
    return -1;
  }
  else
  {
    c = Get_cli(Ol , p->message);
    if(c == NULL)
    {
      Send_Packet(soc , "SERVER" , "OFFLINE" , p->source , 3);
    } else {
      Send_Packet(soc , "SERVER" , "ONLINE" , p->source , 3);
    }
  }
}

//The Client Handler
void *CLI_HANDLER(void *arg)
{
  //getting socket
  int soc; 
  //for debugging
  int st;
  //Recived Packet
  struct Packet *rep;
  struct MSQ *t;
  //Client
  struct Client *cli = (Client *)arg;
  rep = (Packet *)malloc(sizeof(Packet));
  soc = cli->sock;
  if(soc==-1)
  {
    fprintf(stderr , "accept error returned -1\n");
    pLog(__func__ , "Error: client %s not found!",rep->message);
    exit(-1);
  }
  while(1){
    rep = Recive_Packet(soc);
    if(rep == NULL)
    {
      pLog(__func__ , "Connection Lost!");
      pthread_exit(0);
    }    
    else
    {
      switch(rep->type)
      {
	case 1 : {
	  st = SessionMGR(soc , rep);
	  if(st == 9)
	  {
	    pLog(__func__ , "Thread Execution Complete.");
	    pthread_exit(0);
	  }else if(st == -1)
	  {
	    pLog(__func__ , "Connection Lost!");
	    pthread_exit(0);
	  }
	}
	break;
	case 3 : {
	  st = ClientsMGR(soc , rep);
	  if(st == 9)
	  {
	    pLog(__func__ , "Thread Execution Complete.");
	    close(soc);
	    Rem_cli(Ol , soc , &count_cli);
	    
	    update("None");
	    pthread_exit(0);
	  }else if(st == -1)
	  {
	    pLog(__func__ , "Connection Lost!");
	    pthread_exit(0);
	  }
	}
	break;
	case 0 : Route(rep);
	break;
	default : 
	{
	  fprintf(stderr , "Unknown Packet\n");
	  pLog(__func__ , "Unknown Packet Recived!");
	}  
      }
    }
  }
}

int main(int argc , char *argv[])
{
  //For Debugging
  int st; 
  //Server Port
  int portserveur;
  //Listening Socket
  int soc; 
  //Client Sockets
  int s[MAX_CLIENTS];
  //Client Handlers
  pthread_t th[MAX_CLIENTS];
  //Return of fork() multi clients
  int multi; 
  //length of client structure
  socklen_t lgclient;
  //server struct
  struct sockaddr_in serveur;
  //client struct
  struct sockaddr_in client;
  //Recived Packet
  struct Packet *rep;
  //client struct 
  struct Client *c;
  //Recived Message
  char response[300];
  //Split results
  char **ar;
  
  l = NULL;
  Ol = NULL;
  rep = (Packet *)malloc(sizeof(Packet));
  printf("\t\t\t<<<<\tInitializing\t>>>>\t\t\t\n\n");
  pLog(__func__ , "Loading Clients...");
  l = Load_CDFs("./Clients");
  pLog(__func__ , "Loading Complete!");
  if(argc != 2)
  {
    fprintf(stderr , "Usage: %s <port>\n",argv[0]);
    pLog(__func__ , "Invalid Arguments1");
    return -1;
  }	  
  portserveur = atoi(argv[1]);
  printf("Starting TCP Chat Server.....                        \n");
  printf("Done.\n");
  if(portserveur <=0 || portserveur > 65535 || !isfreeport(portserveur))
  {
    fprintf(stderr , "Invalid Port!\n");
    pLog(__func__ , "Invalid Port!");
    return -1;
  }
  printf("\t\t\t<<<<\tCreating listening Socket\t>>>>\t\t\t\n\n");
  pLog(__func__ , "Creating Socket....\t\t");
  printf("Creating Socket...\t\t");
  soc = socket(AF_INET,SOCK_STREAM,6);
  if(soc == -1)
  {
    printf("[ err ]\n");
    return(-1);
  } else printf(" [ ok ]\n");
  serveur.sin_family=AF_INET;
  serveur.sin_port = htons(portserveur);
  serveur.sin_addr.s_addr= inet_addr("127.0.0.1");
  pLog(__func__ , "Binding socket %d with 127.0.0.1 and port %d",soc,portserveur);
  st = bind(soc,(const struct sockaddr *)&serveur,sizeof(serveur));
  if(st!=0)
  {    
    return(-1);
  }  
  printf("Done.\n");
  pLog(__func__ , "Atempting to Listen.....\t\t");
  printf("Listening...\t\t");
  st=listen(soc,10);
  if(st!=0)
  {
    printf("[ err ]\n");
    return(-1);
  }
  printf(" [ ok ]\n");
  lgclient = sizeof(client);
  while(1)
  {
    count_cli++;
    s[count_cli] = accept(soc,(struct sockaddr *)&client,&lgclient);
    pLog(__func__ , "accept(): %d" , s[count_cli]);
    rep = Recive_Packet(s[count_cli]);
    if(rep != NULL)
    {
      if(rep->message == NULL)
	printf("NULL :(");
      else if(strcmp(rep->message , "LOGIN_REQ") == 0)
      {
	if(count_cli < 99)
	{
	  rep = Recive_Packet(s[count_cli]);
	  if(rep == NULL)
	  {
	    pLog(__func__ , "Connection Lost!");
	    continue;
	  }
	  ar = str_split(rep->message , "&" , 2);
	  c = Get_cli(l , ar[0]);
	  if(c == NULL)
	  {
	    Send_Packet(s[count_cli], "SERVER" , "LOGIN_REJ" , rep->source , 1);
	    fprintf(stderr , "Client %s NOT FOUND\n" , ar[0]);
	    continue;
	  }
	  else if(strcmp(ar[1] , c->pass) != 0)
	  {
	    Send_Packet(s[count_cli] , "SERVER" , "LOGIN_REJ" , rep->source , 1);
	    fprintf(stderr , "PASSWORD INCORRECT\n");
	    continue;
	  } else {
	    st = Send_Packet(s[count_cli] , "Server" , "LOGIN_ACK" , rep->source , 1);
	    if(st <= 0)
	    {
	      fprintf(stderr , "Error sending LOGIN_ACK\n");
	      continue;
	    }
	  }
	  Ol = insert_cli(Ol , rep->source , ar[1] , s[count_cli] , NULL/* , &client */);
	  if(Ol == NULL)
	  {
	    pLog(__func__ , "Ol NULL :(\n");
	    return -1;
	  }
	  printf("Logged On: %s\n" , rep->source);
	  update(rep->source);
	  printf("\t\t\t<<<<\tCreating thread\t>>>>\t\t\t\n\n");
	  c->sock = s[count_cli];
	  pthread_create(&th[count_cli] , NULL , &CLI_HANDLER , c);
	  printf("Done\n\n");
	} else {
	  st = Send_Packet(s[count_cli] , "SERVER" , "LOGIN_REJ" , rep->source , 1);
	  fprintf(stderr , "Client Rejected!\n");
	  pLog(__func__ , "Client Rejected!");
	}
      }
      else
      {
	fprintf(stderr , "Error: Expected LOGIN_REQ but recived %s\n",rep->message);
	pLog(__func__ , "Error: Expected LOGIN_REQ but recived %s",rep->message);
	
      }     
    } 
    else
    {
      fprintf(stderr , "Error Reciving Login Request!\n");
      pLog(__func__ , "Error Reciving Login Request!");
    }
  }  
  close(soc);
}