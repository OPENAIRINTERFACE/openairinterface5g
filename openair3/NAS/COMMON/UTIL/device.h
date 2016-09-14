/*****************************************************************************
Source    device.h

Version   0.1

Date    2012/11/29

Product   NAS stack

Subsystem Utilities

Author    Frederic Maurel

Description Implements Linux/UNIX I/O device handlers

*****************************************************************************/
#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <sys/types.h>

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* Type of the connection endpoint */
#define DEVICE    3

/* Hidden structure that handles device data */
typedef struct device_id_s device_id_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void* device_open(int type, const char* devname, const char* params);
int device_get_fd(const void* id);

ssize_t device_read(void* id, char* buffer, size_t length);
ssize_t device_write(const void* id, const char* buffer, size_t length);

void device_close(void* id);

#endif /* __DEVICE_H__*/
