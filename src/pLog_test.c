#include <stdio.h>
#include <stdarg.h>
 
void error( const char* format, ... ) 
{
  va_list args;
  fprintf( stderr, "Error: " );
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  fprintf( stderr, "\n" );
}

int main()
{
  int i = 1;
  error("The value of i is %d\n" , i);
  return 0;
}