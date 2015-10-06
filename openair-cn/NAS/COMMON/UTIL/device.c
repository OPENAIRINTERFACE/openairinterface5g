/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
/*****************************************************************************
Source    device.c

Version   0.1

Date    2012/11/29

Product   NAS stack

Subsystem Utilities

Author    Frederic Maurel

Description Implements Linux/UNIX I/O device handlers

*****************************************************************************/

#include "device.h"
#include "commonDef.h"

#include <stdio.h>  // fflush
#include <stdlib.h> // malloc, free
#include <string.h> // strncpy
#include <unistd.h> // read, write, close

#include <sys/stat.h>
#include <fcntl.h>  // open

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/* Function used to configure TTY I/O terminal attributes */
extern int stty_set(int fd, const char *params);

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* ------------------------
 * Identifier of the device
 * ------------------------
 *  A device is defined with a pathname and attribute parameters.
 *  A file descriptor is created to handle the setup of the device
 *  attributes.
 */
struct device_id_s {
#define DEVICE_PATHNAME_SIZE  32
  char pathname[DEVICE_PATHNAME_SIZE+1]; /* device pathname   */
  int fd;          /* device file descriptor  */
};

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  device_open()                                             **
 **                                                                        **
 ** Description: Open a device to get a file descriptor for use with read  **
 **    and write I/O operations                                  **
 **                                                                        **
 ** Inputs:  devpath: Device path name                           **
 **    params:  Device parameters                          **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:        A pointer to the device identifier alloca- **
 **       ted for I/O operations. NULL if the device **
 **       has not been successfully opened.          **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
void* device_open(int type, const char* devpath, const char* params)
{
  int fd;

  /* Parameters sanity check */
  if (devpath == NULL) {
    return NULL;
  }

  if (type != DEVICE) {
    return NULL;
  }

  /* Open the device for I/O operations */
  fd = open(devpath, O_RDWR);

  if (fd < 0) {
    return NULL;
  }

  /* The device has been successfully opened */
  device_id_t * devid = (device_id_t *)malloc(sizeof(struct device_id_s));

  if (devid != NULL) {
    strncpy(devid->pathname, devpath, DEVICE_PATHNAME_SIZE);
    devid->fd = fd;

    if (params != NULL) {
      /* Set TTY parameters */
      if (stty_set(fd, params) != RETURNok) {
        device_close(devid);
        devid = NULL;
      }
    }
  }

  return devid;
}

/****************************************************************************
 **                                                                        **
 ** Name:  device_close()                                            **
 **                                                                        **
 ** Description: Close the specified device and frees memory space alloca- **
 **    ted to handle access to the device                        **
 **                                                                        **
 ** Inputs:  None                                                      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   id:    The identifier of the device               **
 **      Return:  None                                       **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
void device_close(void* id)
{
  if (id) {
    close( ((device_id_t*)id)->fd );
    free(id);
  }
}

/****************************************************************************
 **                                                                        **
 ** Name:  device_read()                                             **
 **                                                                        **
 ** Description: Read data from the given I/O device into the input buffer **
 **    of specified length                                       **
 **                                                                        **
 ** Inputs:  id:    The identifier of the device               **
 **      len:   The number of bytes to read                **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  The input buffer                           **
 **      Return:  The number of bytes read when success;     **
 **       RETURNerror otherwise.                     **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
ssize_t device_read(void* id, char* buffer, size_t len)
{
  const device_id_t* devid = (device_id_t*)(id);
#if 0
  ssize_t rbytes = read(devid->fd, buffer, len);

  if (rbytes < 0) {
    return RETURNerror;
  }

#endif

  //#if 0
  ssize_t rbytes = 0;

  do {
    ssize_t size = read(devid->fd, buffer + rbytes, len - rbytes);

    if (size < 0) {
      return RETURNerror;
    }

    rbytes += size;
  } while ( (buffer[rbytes-1] != '\r') && (buffer[rbytes-1] != '\n')
            && (buffer[rbytes-1] != '\0') );

  //#endif

  return rbytes;
}

/****************************************************************************
 **                                                                        **
 ** Name:  device_write()                                            **
 **                                                                        **
 ** Description: Write data to the given I/O device from the output buffer **
 **    of specified length                                       **
 **                                                                        **
 ** Inputs:  id:    The identifier of the device               **
 **      buffer:  The output buffer                          **
 **      len:   The number of bytes to write               **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **      Return:  The number of bytes written when success;  **
 **       RETURNerror otherwise.                     **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
ssize_t device_write(const void* id, const char* buffer, size_t len)
{
  const device_id_t* devid = (device_id_t*)(id);
  ssize_t sbytes = write(devid->fd, buffer, len);
  fflush(NULL);

  if (sbytes != len) {
    return RETURNerror;
  }

  return sbytes;
}

/****************************************************************************
 **                                                                        **
 ** Name:  device_get_fd()                                           **
 **                                                                        **
 ** Description: Get the value of the file descriptor created to handle    **
 **    the device with the given identifier                      **
 **                                                                        **
 ** Inputs:  id:    The identifier of the device               **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **      Return:  The file descriptor of the device          **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int device_get_fd(const void* id)
{
  if (id) {
    return ((device_id_t*)id)->fd;
  }

  return RETURNerror;
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

