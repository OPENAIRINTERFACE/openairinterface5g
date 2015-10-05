/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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
