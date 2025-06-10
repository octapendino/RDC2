#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include "serverausftp.h"

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"

/*
 * Recibe un comando desde el socket.
*/

int recv_cmd(int socketDescriptor, char *operation, char *param) {
    char buffer[BUFSIZE];
    char *token;

    if (recv(socketDescriptor, buffer, BUFSIZE, 0) < 0) {
      fprintf(stderr, "Error: no se recibio ningun comando");
      return -1;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    token = strtok(buffer, " ");
    if (token == NULL || strlen(token) < 4) {
        fprintf(stderr, "Error: comando ftp invalido");
        return -1;
    } else {
        strcpy(operation, token);
        token = strtok(NULL, " ");
        #if DEBUG //Al incluir en la compilación -d DEBUG, se van a ver todos los prints q se indiquen de esta forma
        printf("par %s\n", token);
        #endif
        if (token != NULL) strcpy(param, token);
        return 0;
    }
}