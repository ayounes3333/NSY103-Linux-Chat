#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
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
#include "packet.h"

#define MAX_CLIENTS 100

//##########   Client Structures   ##########//

//Structure of a client
typedef struct Client {
  char name[20];
  char pass[30];
  struct MSQ *msq;
  //struct sockaddr_in *ADDR;
  int sock;
} Client;

//List containing details about connected clients
typedef struct lst_clients
{
  struct Client *c;
  struct lst_clients *s;
} lst_clients;

//##########   CDF Structutres   ##########//

//cdf Header
typedef struct cdf_header {
  char name[20];
  char pass[30];
  int cd_length;
} cdf_header;

//Stored Message Format
typedef struct Message {
  char message[256];
  char src[20];
  //date
}Message;

//Message Queue
typedef struct MSQ {
  struct Message mes;
  struct MSQ *s;
} MSQ;

//=============================================================================//

//Print clients
int print_lst_cli(struct lst_clients *l)
{
  struct lst_clients *t;
  if(l == NULL)
    return -1;
  t = l;
  printf("%s$%s: %d\n" , t->c->name , t->c->pass , t->c->sock);
  if(l->s == NULL)
    return 1;
  for(t = l->s; t->s!=NULL; t=t->s)
  {
    printf("%s$%s: %d\n" , t->c->name , t->c->pass , t->c->sock);
  }
  return 0;
}

//Print MSQ
int print_MSQ(struct MSQ *l)
{
  struct MSQ *t;
  if(l == NULL)
    return -1;
  for(t = l; t->s!=NULL; t=t->s)
  {
    printf("%s: %s\n" , t->mes.src , t->mes.message);
  }
  printf("%s: %s\n" , t->mes.src , t->mes.message);
  return 0;
}

//##########  CDF Functions   ##########//

//Insert a new Message to the MSQ
struct MSQ * insert_msg(struct MSQ *l , char *mes , char *src /* , date */)
{
  struct MSQ *t , *k;
  t = (MSQ *)malloc(sizeof(MSQ));
  t->s = NULL;
  if(mes == NULL)
  {
    pLog(__func__ , "Null Message");
    return l;
  }
  sprintf(t->mes.message , "%s" , mes);
  sprintf(t->mes.src , "%s" , src);
  //t->mes.date;
  //if(l != NULL)
  //{
  //  for(k = l; k->s != NULL; k=k->s);
  //  k->s = t;
  //  return l;
  //}
  t->s = l;
  return t;
}

//Copy MSQ
struct MSQ *msq_cpy(struct MSQ *dest , struct MSQ *src)
{
  struct MSQ *tmp;
  if(src == NULL)
  {
    pLog(__func__ , "Source MSQ is NULL!");
    dest = NULL;
    return NULL;
  }
  for(tmp = src; tmp->s!=NULL; tmp = tmp->s)
  {
    dest = insert_msg(dest , tmp->mes.message , tmp->mes.src);
    pLog(__func__ , "Copied: %s: %s" , tmp->mes.src , tmp->mes.message);
  }
  dest = insert_msg(dest , tmp->mes.message , tmp->mes.src);
  pLog(__func__ , "Copied: %s: %s" , tmp->mes.src , tmp->mes.message);
  
  return dest;
}

//Get a client from the list
struct Client * Get_cli(struct lst_clients *l , char *n)
{
  struct lst_clients *t;
  struct Client *c;
  c = (Client *)malloc(sizeof(Client));
  c->msq = (MSQ *)malloc(sizeof(MSQ));
  if(l == NULL)
  {
    fprintf(stderr , "No clients in the list\n");
    pLog(__func__ , "No clients in the list!");
    return NULL;
  }
  t = l;
  do
  {
    pLog(__func__ , "Comparing %s with %s" , t->c->name , n);
    if(strcmp(t->c->name , n) == 0)
    {
      pLog(__func__ , "%s Equal %s" , t->c->name , n);
      //c->ADDR = t->c->ADDR;
      sprintf(c->name , "%s" , t->c->name);
      sprintf(c->pass , "%s" , t->c->pass);
      c->sock = t->c->sock; 
      c->msq = msq_cpy(c->msq , t->c->msq);
      return c;
    } else 
     pLog(__func__ , "Not Equal (%d)" , strcmp(t->c->name , n));
    t = t->s;
  } while(t != NULL);
  return NULL;
}

//Extract and Print all Messages comming from 'src' from the MSQ
struct MSQ *Ext_msgs_for_cli(int soc , struct Client *cli , char *src)
{
  struct MSQ *p , *q;
  struct MSQ *msq;
  msq = cli->msq;
  if(msq == NULL)
  {
    pLog(__func__ , "NULL MSQ!");
    return NULL;
  }
  if(strcmp(msq->mes.src , cli->name) == 0)
  {    
    pLog(__func__ , "%s: %s\n" , msq->mes.src , msq->mes.message);
    Send_Packet(soc , msq->mes.src , msq->mes.message , src , 0);
    msq=msq->s;
  }
  for(p=msq , q=msq->s; q != NULL; p=p->s , q=q->s)
  {
    if(strcmp(q->mes.src , cli->name) == 0)
    {
      pLog(__func__ , "%s: %s\n" , q->mes.src , q->mes.message);
      Send_Packet(soc , q->mes.src , q->mes.message , src , 0);
      p->s = q->s;
      free(q);
    }
    pLog(__func__ , "Extraction Complete!");
  }
  return msq;
}

//Get the file extension
const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

//Get MSQ length
int MSQ_Len(struct MSQ *msq)
{
  struct MSQ *t;
  int c = 0;
  for(t = msq; t->s!= NULL; t=t->s)
    c++;
  return c;
}

int Save_Client_E(struct Client *cli)
{
  char path[] = "/Clients";
  struct Message mes;
  struct cdf_header h;
  struct MSQ *t;
  char fname[100];
  int i , st;
  FILE *f;
  if(cli != NULL)
  {
    sprintf(fname , "%s.cdf" , cli->name);
    chdir(path);
    f = fopen(fname , "w+");
    if(f == NULL)
    {
      fprintf(stderr , "Error opening %s for write\n" , fname);
      pLog(__func__ , "Error Openng %s for write!" , fname);
      if(errno == EINVAL)
	pLog(__func__ , "parameter error");
      fprintf(stderr , "%d\n" , errno);
      fclose(f);
      return -1;
    }
    sprintf(h.name , "%s" , cli->name);
    sprintf(h.pass , "%s" , cli->pass);
    h.cd_length = 0;
    pLog(__func__ , "Saving %s(%s) %d Messages" , h.name , h.pass , h.cd_length);
    st = fwrite(&h , sizeof(h) , 1 , f);
    pLog(__func__ , "SizeOf Header: %d, Wrote: %d" , sizeof(h) , st);
    if(st == -1)
    {
      fprintf(stderr , "Error Writing Header for %s\n" , cli->name);
      pLog(__func__ , "Error Writing Header for %s!" , cli->name);
      fclose(f);
      return -1;
    }
    fclose(f);
  }
}

//Save Client to file
int Save_Client(struct Client *cli)
{
  char path[] = "/Clients";
  struct Message mes;
  struct cdf_header h;
  struct MSQ *t;
  char fname[100];
  int i , st;
  FILE *f;
  if(cli != NULL)
  {
    sprintf(fname , "%s.cdf" , cli->name);
    chdir(path);
    f = fopen(fname , "w+");
    if(f == NULL)
    {
      fprintf(stderr , "Error opening %s for write\n" , fname);
      pLog(__func__ , "Error Openng %s for write!" , fname);
      if(errno == EINVAL)
	pLog(__func__ , "parameter error");
      fprintf(stderr , "%d\n" , errno);
      fclose(f);
      return -1;
    }
    sprintf(h.name , "%s" , cli->name);
    sprintf(h.pass , "%s" , cli->pass);
    h.cd_length = MSQ_Len(cli->msq);
    pLog(__func__ , "Saving %s(%s) %d Messages" , h.name , h.pass , h.cd_length);
    st = fwrite(&h , sizeof(h) , 1 , f);
    pLog(__func__ , "SizeOf Header: %d, Wrote: %d" , sizeof(h) , st);
    if(st == -1)
    {
      fprintf(stderr , "Error Writing Header for %s\n" , cli->name);
      pLog(__func__ , "Error Writing Header for %s!" , cli->name);
      fclose(f);
      return -1;
    }
    if(cli->msq != NULL)
    {
      for(t = cli->msq; t->s != NULL; t = t->s)
      {
	st = fwrite(&(t->mes) , sizeof(t->mes) , 1 , f);
	if(st == -1)
	{
	  fprintf(stderr , "Error writing MSQ to cdf\n");
	  pLog(__func__ , "Error writing MSQ to cdf!");
	  fclose(f);
	  return -1;
	}
      }
      st = fwrite(&(t->mes) , sizeof(t->mes) , 1 , f);
      if(st == -1)
	{
	  fprintf(stderr , "Error writing MSQ to cdf\n");
	  pLog(__func__ , "Error writing MSQ to cdf!");
	  fclose(f);
	  return -1;
	}      
    }
    fclose(f);
  }
}

//##########   Client Functions   ##########//

//Send the client list
int Send_clis(int soc , struct lst_clients *l)
{
  int st , count = 0;
  struct lst_clients *p;
  char *mes , rep[300];
  mes = (char *)malloc(20);
  for(p = l; p->s != NULL; p = p->s)
  {
    count ++;
  }
  st = send(soc , &count , sizeof(int) , 0);
  for(p = l; p->s != NULL; p=p->s)
  {
    mes = strcpy(mes , p->c->name);
    st = send(soc,mes,strlen(mes),0);
    if(st!=strlen(mes))
    {
      fprintf(stderr , "Error Sending a client\n");
      pLog(__func__ , "Error Sending a client!");
      return -1;
    }
    pLog(__func__ , "%s Sent" , mes);
    sleep(1);
  }
  sprintf(mes , "END_OF_CLIENTS");
  st = send(soc,mes,strlen(mes),0);
  if(st!=strlen(mes))
  {
    fprintf(stderr , "Error sending END_OF_CLIENTS\n");
    pLog(__func__ , "Error sending END_OF_CLIENTS!");
    return -1;    
  }
  return 0;
}

//Remove a client from the list
struct lst_clients *Rem_cli(struct lst_clients *l , int soc , int *c)
{
  struct lst_clients *p , *q;
  if(l->c->sock == soc)
  {
    *c--;
    l=l->s;
    return l;
  }
  for(p=l , q=l->s; q->s != NULL; p=p->s , q=q->s)
  {
    if(q->c->sock == soc)
    {
      p->s = q->s;
      free(q);
      return l;
    }
  }
}



//Insert a new client to the list
struct lst_clients * insert_cli(struct lst_clients *l , char *n , char *p , int soc , struct MSQ *msq/* , struct sockaddr_in *cli */)
{
  struct lst_clients *t;
  t = (lst_clients *)malloc(sizeof(lst_clients));
  t->s = l;
  t->c = (Client *)malloc(sizeof(Client));
  t->c->msq = (MSQ *)malloc(sizeof(MSQ));
  sprintf(t->c->name , "%s" , n);
  sprintf(t->c->pass , "%s" , p);
  t->c->msq = msq_cpy(t->c->msq , msq);
  //t->c->ADDR = cli;
  t->c->sock = soc;
  pLog(__func__ , "Inserted: %s(%s)", t->c->name , t->c->pass);
  return t;
}

//Get clients from file
struct lst_clients *Load_CDFs(char *path)
{
  char *fn;
  struct lst_clients *l;
  struct Message *mes;
  struct MSQ *msq;
  struct cdf_header *h;
  DIR *d;
  struct dirent *dir;
  int st , i;
  FILE *f;
  fn = (char *)malloc(100);
  l = (lst_clients *)malloc(sizeof(lst_clients));
  l->s = NULL;
  mes = (Message *)malloc(sizeof(Message));
  msq = NULL;
  h = (cdf_header *)malloc(sizeof(cdf_header));
  d = opendir(path);
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      if(strcmp(get_filename_ext(dir->d_name),"cdf") == 0)
      {
	pLog(__func__ , "Loaded: %s" , dir->d_name);
	msq = (MSQ *)malloc(sizeof(MSQ));
	msq->s = NULL;
	sprintf(fn , "%s/%s" , path , dir->d_name);
	f = fopen(fn, "r");
	if(f == NULL)
	{
	  fprintf(stderr , "Error opening file %s\n" , dir->d_name);
	  pLog(__func__ , "Error opening file %s!" , dir->d_name);
	  return NULL;
	}
	st = fread(h , sizeof(*h) , 1 , f);
	if(st == -1)
	{
	  fprintf(stderr , "Error Reading %s's Header!\n" , fn);
	  pLog(__func__ , "Error Reading %s's Header!" , fn);
	  return NULL;
	}
	pLog(__func__ , "Header Loaded: %s(%s) %d Messages" , h->name , h->pass , h->cd_length);
	for(i = 0; i < h->cd_length; i++)
	{
	  st = fread(mes , sizeof(*mes) , 1 , f);
	  if(st == -1)
	  {
	    fprintf(stderr , "Error reading from %s\n" , fn);
	    pLog(__func__ , "Error reading from %s!" , fn);
	    return NULL;
	  }
	  msq = insert_msg(msq , mes->message , mes->src);
	}
	l = insert_cli(l , h->name , h->pass , -1 , msq);
	fclose(f);
      }      
    }    
    closedir(d);
    return l;
  }
}

//Recive Clients
char **RCV_CLIs(int soc , int *c)
{
  char rep[300];
  *c = 0;
  int st , count;
  char **ar;
  st = recv(soc,&count,sizeof(int),0);
  if(st > 0)
  {
    printf("%d Clients\n" , count);
  } else {
    fprintf(stderr , "Error Reciving clients Number\n");
    return NULL;
  }
  ar = malloc(sizeof(char *) * count);
  while(1)
  {
    st = recv(soc,&rep,20,0);
    if(st > 0)
    {
      rep[st] = 0;
      pLog(__func__ , "Recived %s\n" , rep);
      ar[*c] = (char *)malloc(strlen(rep));
      if(strcmp(rep , "END_OF_CLIENTS") == 0)
      {
	*c = count;
	return ar;	
      }
      strcpy(ar[*c] , rep);
      *c++;
    } else {
      fprintf(stderr , "Error Reciving clients list\n");
      return NULL;
    }
  }
}

