#include <stdio.h>
#include <stdlib.h>

int main()
{
  int *p , *q;
  int a = 55;
  *p = a;
  q = p;
  free(p);
  printf("is %d = %d?" , *q , a);
}