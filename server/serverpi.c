#include "serverpi.h"
#include "serverausftp.h"
#include "responses.h"
#include "utils.h" 
#include "session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <strings.h>

static ftp_command_t ftp_commands[] = {
    {"USER", handle_USER},
    {"PASS", handle_PASS},
    {"QUIT", handle_QUIT},
    {"SYST", handle_SYST},
    {"TYPE", handle_TYPE},
    {"PORT", handle_PORT},
    {"RETR", handle_RETR},
    {"STOR", handle_STOR},
    {"NOOP", handle_NOOP},
    {NULL, NULL}
};


int welcome(ftp_session_t *sess){
    //mando mensaje FTP bienvenida
    if(safe_dprintf(sess->control_sock, MSG_220) != sizeof(MSG_220) -1){ 
        fprintf(stderr, "Send error\n");
        close_fd(sess->control_sock, "cliente socket"); 
        return -1;
    }
    return 0;
}

int get_exe_command(ftp_session_t *sess){
    char buffer[BUFSIZE];

    //recive string del command chanel
    ssize_t len = recv(sess->control_sock, buffer, sizeof(buffer) -1, 0);
    if(len < 0){
        perror("Receive fail: ");
        close_fd(sess->control_sock, "cliente socket");
        return -1;
    }

    //la conexion se cerro inapropiadamente y nosotros la cerramos bien
    if(len == 0){
        sess->current_user[0] = '\0'; //cierro sesion
        close_fd(sess->control_sock, "client socket");
        sess->control_sock = -1;
        return -1;
    }

    buffer[len] = '\0';

    //strip CRLF
    char *cr = strchr(buffer, '\r');
    if(cr) *cr = '\0';
    char *lf = strchr(buffer, '\n');
    if(lf) *lf = '\0';

    //separo comando y argumento
    char *arg = NULL;
    char *cmd = buffer;

    //caso de comando nulo
    if(cmd[0] == '\0'){
        safe_dprintf(sess->control_sock, "500 Empty command. \r\n");
        return 0;
    }

    char *space = strchr(buffer, ' ');
    if(space){
        *space = '\0';
        arg = space + 1;
        while(*arg == ' ') arg++;
    }

    ftp_command_t *entry = ftp_commands;
    while(entry->name){
        if(strcasecmp(entry->name, cmd) == 0){
            entry->handler(arg ? arg : "");
            return (sess->control_sock < 0) ? -1 : 0;
        }
        entry++;
    }

    safe_dprintf(sess->control_sock, "502 Command not implemented.\r\n");
    return 0;

}