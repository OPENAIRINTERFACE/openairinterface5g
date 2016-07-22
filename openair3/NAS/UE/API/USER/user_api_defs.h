#ifndef _USER_API_DEFS_H
#define _USER_API_DEFS_H

#include <sys/types.h>
#include "at_command.h"

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

#define USER_API_RECV_BUFFER_SIZE 4096
#define USER_API_SEND_BUFFER_SIZE USER_API_RECV_BUFFER_SIZE
#define USER_DATA_MAX 10

/*
 * The decoded data received from the user application layer
 */
typedef struct {
  int n_cmd;    /* number of user data to be processed    */
  at_command_t cmd[USER_DATA_MAX];  /* user data to be processed  */
} user_at_commands_t;

/* -------------------
 * Connection endpoint
 * -------------------
 *      The connection endpoint is used to send/receive data to/from the
 *      user application layer. Its definition depends on the underlaying
 *      mechanism chosen to communicate (network socket, I/O terminal device).
 *      A connection endpoint is handled using an identifier, and functions
 *      used to retreive the file descriptor actually allocated by the system,
 *      to receive data, to send data, and to perform clean up when connection
 *      is shut down.
 *      Only one single end to end connection with the user is managed at a
 *      time.
 */
typedef struct {
  /* Connection endpoint reference  */
  void* endpoint;
  /* Connection endpoint handlers */
  void*   (*open) (int, const char*, const char*);
  int     (*getfd)(const void*);
  ssize_t (*recv) (void*, char*, size_t);
  ssize_t (*send) (const void*, const char*, size_t);
  void    (*close)(void*);
  char    recv_buffer[USER_API_RECV_BUFFER_SIZE];
  char    send_buffer[USER_API_SEND_BUFFER_SIZE];
} user_api_id_t;


#endif
