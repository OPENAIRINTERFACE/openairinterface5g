/*****************************************************************************
Source    socket.h

Version   0.1

Date    2012/02/28

Product   NAS stack

Subsystem Utilities

Author    Frederic Maurel

Description Implements TCP socket handlers

*****************************************************************************/
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/types.h>

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* Type of the connection endpoint */
#define SOCKET_CLIENT 1
#define SOCKET_SERVER 2

/* Hidden structure that handles the connection endpoint data */
typedef struct socket_id_s socket_id_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void* socket_udp_open(int type, const char* host, const char* port);
int socket_get_fd(const void* id);

ssize_t socket_recv(void* id, char* buffer, size_t length);
ssize_t socket_send(const void* id, const char* buffer, size_t length);

void socket_close(void* id);

#endif /* __SOCKET_H__*/
