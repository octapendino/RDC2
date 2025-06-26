#include "responses.h"
#include "pi.h"
#include "dtp.h"
#include "session.h" 
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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
        safe_dprintf(sess->control_sock, MSG_503); //mala secuencia de comandos
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

    if (!args || strlen(args) != 1) {
    	safe_dprintf(sess->control_sock, MSG_501);
    	return;
    }

    if(args[0] == 'I'){
        safe_dprintf(sess->control_sock, MSG_200); //Solo aceptamos modo binario
    }else if(args[0] == 'A' || args[0] == 'E' || args [0] == 'L'){
        safe_dprintf(sess->control_sock, MSG_504); //Otro modo indicamos no implementado
    }else{
        safe_dprintf(sess->control_sock, MSG_501);//Modo invaido, error de sintaxis
    }
}

void handle_PORT(const char *args){
    ftp_session_t *sess = session_get();

    if (!args || strlen(args) == 0) { 
        safe_dprintf(sess->control_sock, MSG_501); 
        return;
    }

    int h1, h2, h3, h4, p1, p2;
    if (sscanf(args, "%d,%d,%d,%d,%d,%d", 
               &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        safe_dprintf(sess->control_sock, MSG_501); //Si no puede guardar la IP y el Puerto
        return;
    }

    if ((h1|h2|h3|h4) < 0 || h1 > 255 || h2 > 255 || h3 > 255 || h4 > 255 ||
        p1 < 0 || p1 > 255 || p2 < 0 || p2 > 255) {
        safe_dprintf(sess->control_sock, MSG_501); //Si no se encuentran dentro del rango permitido (0-256)
        return;
    }

    //Armamos la IP y Puerto donde va a escuchar el cliente
    char ip[INET_ADDRSTRLEN];
    snprintf(ip, sizeof(ip), "%d.%d.%d.%d", h1, h2, h3, h4);
    int port = (p1 * 256) + p2;

    //Seteamos el canal de datos en la sesión activa
    memset(&sess->data_addr, 0, sizeof(sess->data_addr));
    sess->data_addr.sin_family = AF_INET;
    sess->data_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &sess->data_addr.sin_addr);

    safe_dprintf(sess->control_sock, "200 PORT command successful.\r\n");
}

void handle_RETR(const char *args){
    ftp_session_t *sess = session_get();

    if (!sess->logged_in) {
        safe_dprintf(sess->control_sock, MSG_530); //Verificamos que el usuario este logueado
        return;
    }

    if (!args || strlen(args) == 0) {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    //Abrimos el archivo para lectura
    int fd = open(args, O_RDONLY);
    if (fd < 0) {
        perror("open");
        safe_dprintf(sess->control_sock, MSG_550); //Error al abrir el archivo
        return;
    }

    //Creamos el socket de datos del servidor para conectarnos al socket del cliente
    sess->data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sess->data_sock < 0) {
        perror("socket");
        safe_dprintf(sess->control_sock, MSG_425); //Error al crear el socket
        close(fd);
        return;
    }

    //Nos conectamos al cliente, a partir de la info guardada con PORT en data_addr
    if (connect(sess->data_sock, (struct sockaddr*)&sess->data_addr, sizeof(sess->data_addr)) < 0) {
        perror("connect");
        safe_dprintf(sess->control_sock, MSG_425);//Error al establecer la conexion
        close_fd(sess->data_sock, "data socket");
        sess->data_sock = -1;
        close(fd);
        return;
    }

    //Listo para enviar datos
    safe_dprintf(sess->control_sock, MSG_150);

    //Enviamos el archivo por el socket de datos
    char buf[BUFSIZE];
    ssize_t r;
    while ((r = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t sent = 0;
        while (sent < r) {
            ssize_t n = send(sess->data_sock, buf + sent, r - sent, 0);
            if (n < 0) {
                perror("send");
                safe_dprintf(sess->control_sock, MSG_426);//Error enviando el archivo
                close(fd);
                close_fd(sess->data_sock, "data socket");
                sess->data_sock = -1;
                return;
            }
            sent += n;
        }
    }

    //Cerramos socket de datos abierto y archivo
    close(fd);
    close_fd(sess->data_sock, "data socket");
    sess->data_sock = -1;

    safe_dprintf(sess->control_sock, MSG_226);
}

void handle_STOR(const char *args){
    ftp_session_t *sess = session_get();

    if (!sess->logged_in) {
        safe_dprintf(sess->control_sock, MSG_530); //Verificamos que el usuario este logueado
        return;
    }

    if (!args || strlen(args) == 0) {
        safe_dprintf(sess->control_sock, MSG_501);
        return;
    }

    //Creamos el archivo para escritura con el nombre que se recibe como argumento
    int fd = open(args, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open");
        safe_dprintf(sess->control_sock, MSG_553); //Error al crear el archivo
        return;
    }

    //Creamos el socket de datos del servidor para conectarnos al socket del cliente
    sess->data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sess->data_sock < 0) {
        perror("socket");
        safe_dprintf(sess->control_sock, MSG_425); //Error al crear el socket
        close(fd);
        return;
    }

    ////Nos conectamos al cliente, a partir de la info guardada con PORT en data_addr
    if (connect(sess->data_sock, (struct sockaddr*)&sess->data_addr, sizeof(sess->data_addr)) < 0) {
        perror("connect");
        safe_dprintf(sess->control_sock, MSG_425);
        close_fd(sess->data_sock, "data socket");
        sess->data_sock = -1;
        close(fd);
        return;
    }

    //Listo para recibir datos
    safe_dprintf(sess->control_sock, MSG_150);

    //Leemos del socket de datos y escribimos en el archivo
    char buf[BUFSIZE];
    ssize_t n;
    while ((n = recv(sess->data_sock, buf, sizeof(buf), 0)) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(fd, buf + written, n - written);
            if (w < 0) {
                perror("write");
                safe_dprintf(sess->control_sock, MSG_452); //Error escribiendo archivo
                close(fd);
                close_fd(sess->data_sock, "data socket");
                sess->data_sock = -1;
                return;
            }
            written += w;
        }
    }

    //Cerramos socket de datos abierto y archivo
    close(fd);
    close_fd(sess->data_sock, "data socket");
    sess->data_sock = -1;

    // Transferencia exitosa
    safe_dprintf(sess->control_sock, MSG_226);
}

void handle_NOOP(const char *args){
    ftp_session_t *sess = session_get();
    (void)args;
    (void)sess;

    safe_dprintf(sess->control_sock, MSG_200); //Envio de mensaje sin realizar ninguna operación.

}