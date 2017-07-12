#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

//log an event
int pLog(const char *caller , const char *format , ...)
{
  time_t current_time;
  char *time_string;
  char event[100];
  char Message[100];
  FILE *f;
  va_list args;
  va_start(args, format);
  vsprintf(Message , format, args);
  va_end(args);
  f = fopen("./log.txt" , "a+");
  current_time = time(NULL);
  time_string = ctime(&current_time);
  time_string[strlen(time_string) - 1] = '\0';
  sprintf(event , "[%s] %s(): %s\n" , time_string , caller , Message);
  fwrite(event , 1 , strlen(event) , f);
  fclose(f);
}

//Split a string 
char **str_split(char *str , const char* delim , int ct)
{
  char **at , *saveptr;
  int i;
  char *p;
  at = (char **)malloc(sizeof(char *) * ct);
  pLog(__func__ , "Split \"%s\" in %d tokens", str , ct);
  p = strtok_r(str,delim , &saveptr);
  at[0] = (char *)malloc(strlen(p));
  sprintf(at[0] , "%s" , p);
  for(i=1; ; i++) 
  { 
    pLog(__func__ , "%s", p); 
    p = strtok_r(NULL, delim , &saveptr);
    if(p == NULL)
      break;
    at[i] = (char *)malloc(strlen(p));
    sprintf(at[i] , "%s" , p);
  }
  return at; 
}

//Encrypt a string of text
char *CEncrypt(char *plainText)
{
  int Key = 3 , i;
  char *cipher;
  if(plainText == NULL)
    return NULL;
  cipher = (char *)malloc(strlen(plainText));
  for(i = 0; i < strlen(plainText); i++)
  {
    cipher[i] = (plainText[i] + Key) % 256;
  }
  return cipher;
}

//Decrypt a string of text
char *CDecrypt(char *cipher)
{
  int Key = 3 , i;
  char *plainText;
  if(cipher == NULL)
    return NULL;
  plainText = (char *)malloc(strlen(cipher));
  for(i = 0; i < strlen(cipher); i++)
  {
    plainText[i] = (cipher[i] - Key) % 256;
  }
  return plainText;
}
