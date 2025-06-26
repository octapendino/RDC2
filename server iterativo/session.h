#pragma once

# include "config.h"
#include <netinet/in.h> // sockaddr_in

/*
typedef enum{
  TYPE_ASCII,
  TYPE_BIN
} transfer_type_t;
*/

typedef struct {
  int control_sock;                // Control connection
  int data_sock;                   // Data connection
  struct sockaddr_in data_addr;    // For PORT command
  char current_user[USERNAME_MAX]; // Session username
  uint8_t logged_in;               // 0 = false, 1 = true
  // trasnfer_type_t transfer_type; almacenar en que tipo en que se van a comunicar
} ftp_session_t;

// Global pointer to current session (set by each child process)
extern ftp_session_t *current_sess;

ftp_session_t *session_get(void);
void session_init(int control_fd);
void session_cleanup(void);