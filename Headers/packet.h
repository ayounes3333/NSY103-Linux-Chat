#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include "strtools.h"

//Packet Structure
typedef struct Packet
{
  int type;
  char *source;
  char *destination;
  char *message;
} Packet;


//Serialize a Packet structure
char *Serialize_Packet(struct Packet *p)
{
  char *res;
  res = (char *)malloc(300);
  sprintf(res , "%d$%s$%s$%s" , p->type , p->source , p->destination , p->message);
  //res = CEncrypt(res);
  return res;
}

//Deserialize a Packet
struct Packet *Deserialize_Packet(char *buff)
{
  int count = 0;
  char **ar;
  struct Packet *t;
  t = (Packet *)malloc(sizeof(Packet));
  //buff = CDecrypt(buff);
  ar = str_split(buff , "$" , 4);
  t->type = atoi(ar[0]);
  t->source = ar[1];
  t->destination = ar[2];
  t->message = ar[3];
  return t;
}

//Recive a Packet
struct Packet *Recive_Packet(int soc)
{
  char rep[300];
  struct Packet *p;
  p = (Packet *)malloc(sizeof(Packet));
  int st;
  st = recv(soc,&rep[0],300,0);
  pLog(__func__ , "Recived: %s" , rep);
  if(st > 0)
  {
    rep[st]=0;
    p = Deserialize_Packet(rep);
    return p;
  } 
  else 
  {
    fprintf(stderr , "Connection Lost!\n");
    pLog(__func__ , "Connection Lost!");
    return NULL;
  }
}

//Send a Packet to server
int Send_Packet(int soc , char *src , char *msg , char *dest , int flags)
{
  int st;
  char *mes;
  struct Packet *p;
  p = (Packet *)malloc(sizeof(Packet));
  p->destination = dest;
  p->message = msg;
  p->source = src;
  p->type = flags;
  mes = Serialize_Packet(p);
  pLog(__func__ , "Sending: %s via socket %d" , mes , soc);
  st = send(soc , mes , strlen(mes) , 0);
  if(st != strlen(mes))
  {
    fprintf(stderr , "Error Sending Packet\n");
    pLog(__func__ , "Error Sending Packet!");
    return -1;
  }
  free(p);
  return st;
}