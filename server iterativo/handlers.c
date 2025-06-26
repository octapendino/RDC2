#include "responses.h"
#include "serverpi.h"
#include "serverdtp.h"
#include "session.h" 
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

void handle_USER(const char *args){
    ftp_session_t *sess = session_get();
    
    if(!args || strlen(args) == 0){
        safe_dprintf(sess->control_sock, MSG_501); //error de sintaxis en los parametros
        return;
    }

    strncpy(sess->current_user, args, sizeof(sess->current_user) -1);
    sess->current_user[sizeof(sess->current_user) -1] = '\0';
    safe_dprintf(sess->control_sock, MSG_331); //usuario correcto, se necesita la contraseña
}

void handle_PASS(const char *args){
    ftp_session_t *sess = session_get();
    if(sess->current_user[0] == '\0'){
        safe_sprintf(sess->control_sock, MSG_503); //mala secuencia de comandos
        return;
    }

    if(!args || strlen(args) == 0){
        safe_dprintf(sess->control_sock, MSG_501); //error de sintaxis en los parametros
        return;
    }

    if(check_credentials(sess->current_user, (char *)args) == 0){
        sess->logged_in = 1;
        safe_dprintf(sess->control_sock, MSG_230); //usuario logueado
    }else{
        safe_dprintf(sess->control_sock, MSG_530); // no logueado
        sess->current_user[0] = '\0'; //resetea usuario por login fallido
        sess->logged_in = 0;
    }
}

void handle_QUIT(const char *args){
    ftp_session_t *sess = session_get();
    (void)args; //unused

    safe_dprintf(sess->control_sock, MSG_221); // goodbye
    sess->current_user[0] = '\0'; // cierra sesion
    close_fd(sess->control_sock, "client socket"); //cierra el socket
    sess->control_sock = -1;
}

void handle_SYST(const char *args){
    ftp_session_t *sess = session_get();
    (void)args; //unused

    safe_dprintf(sess->control_sock, MSG_215); // system type
}

void handle_TYPE(const char *args){
    ftp_session_t *sess = session_get();
    (void)args;
    (void)sess;


    if (!args || strlen(args) != 1) {
    	safe_dprintf(sess->control_sock, MSG_501);
    	return;
    }

    if (args[0] == 'I') {
        //sess->transfer_type = TYPE_BIN  para almacenar en que tipo de dato se van a manejar
    	safe_dprintf(sess->control_sock, MSG_200); // Modo binario
    } else if(args[0] == 'A'){
        //sess->transfer_type = TYPE_ASCII
    	safe_dprintf(sess->control_sock, MSG_200); // Modo ASCII
    } else {
    	safe_dprintf(sess->control_sock, MSG_504); // Comando no implementado para ese parámetro
    }

}

void handle_PORT(const char *args){
    ftp_session_t *sess = session_get();
    (void)args;
    (void)sess;


    //placeholder
}

void handle_RETR(const char *args){
    ftp_session_t *sess = session_get();
    (void)args;
    (void)sess;

    //placeholder
}

void handle_STOR(const char *args){
    ftp_session_t *sess = session_get();
    (void)args;
    (void)sess;

    //placeholder
}

void handle_NOOP(const char *args){
    ftp_session_t *sess = session_get();
    (void)args;
    (void)sess;

    safe_dprintf(sess->control_sock, MSG_200); //mensaje sin operacion para no cortar la conexion

}