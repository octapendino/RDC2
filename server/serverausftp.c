#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "serverdtp.h"

#define VERSION "1.0"
#define DEFAULT_PORT 21

int main(int argc, char const *argv[]){

  int port;
  if(argc > 2){
    fprintf(stderr, "Error: Cantidad de argumentos invalidos");
    return -1;
  }
  if(argc == 2){
    port = atoi(argv[1]);
  }else{
    port = DEFAULT_PORT;
  }
  if(port == 0){
    fprintf(stderr, "Error: Puerto invalido");
    return -1;
  }

  printf("%d", port);

  printf("check_credentials devuelve: %d", check_credentials("pepe","pepe"));

    int mastersocket, slavesocket;
    struct sockaddr_in masteraddr, slaveaddr;
    socklen_t slaveaddrlen;

    mastersocket = socket(AF_INET, SOCK_STREAM, 0);

    masteraddr.sin_family = AF_INET;
    masteraddr.sin_addr.s_addr = INADDR_ANY;
    masteraddr.sin_port = htons(port);


    bind(mastersocket, (struct sockaddr *) &masteraddr, sizeof(masteraddr));
    
    listen(mastersocket, 5);

    while(true){
        slaveaddrlen = sizeof(slaveaddr);
        slavesocket = accept(mastersocket, (struct sockaddr *) &slaveaddr, &slaveaddrlen);
        send(slavesocket, "220 1", sizeof("220 1"), 0);
        printf("funciono\n");
    }

    close(mastersocket);


  return 0;
}