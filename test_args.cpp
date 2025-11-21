#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    
   if( argc == 3 )
   {
        char *host_address = argv[1];
        int host_port = atoi(argv[2]);
        printf("bind ip: %s \n", host_address);
        printf("port: %d \n", host_port);
   }
   else if( argc > 3 )
   {
      printf("Too many arguments supplied.\n");
   }
   else
   {
      printf("One argument expected.\n");
   }

  return 0;
}