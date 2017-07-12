#include <stdio.h>
#include "../Headers/packet.h"

int main()
{
  int i , count = 0;
  char **ar;
  char *str;
  str = (char *)malloc(256);
  printf("Text: ");
  scanf("%s" , str);
  ar = str_split(str , ' ' , &count);
  for(i = 0; i< count; i++)
  {
    printf("%s\n" , ar[i]);
  }
}