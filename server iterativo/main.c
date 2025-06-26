//#include "config.h"
#include "arguments.h"
#include "serverausftp.h" //lo comento para hacer pruebas
#include "utils.h"
//#include "signals.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>





int main(int argc, char *argv[]){
    struct arguments args; 

    if(parse_arguments(argc, argv, &args) != 0){
        return EXIT_FAILURE;
    }

    printf("Start server on %s:%d\n", args.address, args.port); //Es para debuggear.

    int master_socket = server_init(args.address, args.port); //Inicia el servidor y coloca el socket en modo escucha.

    if(master_socket < 0){
        return EXIT_FAILURE;
    }

    setup_signals();

    while(1){
        struct sockaddr_in slave_addr;
        int slave_socket = server_accept(master_socket, &slave_addr); //Acepta conexiones en bucle y devuelve un socket en slave_socket.

        if(slave_socket < 0){
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &slave_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Connection from %s:%d accepted\n", client_ip, ntohs(slave_addr.sin_port));

        server_loop(slave_socket); //Gestiona toda la comunicación.

        printf("Connection from %s:%d closed\n", client_ip, ntohs(slave_addr.sin_port));
    }

    close_fd(master_socket, "Socket escuchando");

    return EXIT_SUCCESS;
}