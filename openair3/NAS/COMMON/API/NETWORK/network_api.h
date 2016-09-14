/*****************************************************************************

Source    network_api.h

Version   0.1

Date    2012/03/01

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Implements the API used by the NAS layer to send/receive
    message to/from the network layer

*****************************************************************************/
#ifndef __NETWORK_API_H__
#define __NETWORK_API_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int network_api_initialize(const char* host, const char* port);

int network_api_get_fd(void);
const void* network_api_get_data(void);

int network_api_read_data(int fd);
int network_api_send_data(int fd, int length);
void network_api_close(int fd);

int network_api_decode_data(int length);
int network_api_encode_data(void* data);

#endif /* __NETWORK_API_H__*/
