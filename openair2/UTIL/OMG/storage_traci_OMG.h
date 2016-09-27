/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file storage_traci_OMG.h
* \brief The data storage object carrying data from/to SUMO. 'C' reimplementation of the TraCI version of simITS (F. Hrizi, fatma.hrizi@eurecom.fr)
* \author  S. Uppoor
* \date 2012
* \version 0.1
* \company INRIA
* \email: sandesh.uppor@inria.fr
* \note
* \warning
*/

#ifndef STORAGE_TRACI_OMG_H
#define STORAGE_TRACI_OMG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include "omg.h"

// sortation of bytes forwards or backwards?------------------

extern bool bigEndian;

union n {
  short   s;
  char    c[sizeof(short)];
} un ;


void check_endianness(void);

//----------------STORAGE------------------------------------
struct Storage {

  unsigned char item;
  struct Storage *next;

} ;
typedef struct Storage storage;
// pointer which always points to next entry to be read
// updated in readChar function in storage_traci_OMG

storage *tracker;
storage *head;
storage *storageStart;
int descLen;

extern int msgLength;

void reset(void);
int storageLength(storage *);

void rearange(void);
unsigned char readChar(void);
void writeChar(unsigned char);

int readByte(void) ;
void writeByte(int) ;


int readUnsignedByte(void);
void writeUnsignedByte(int);

char * readString(void) ;
void writeString(char *);

string_list* readStringList(string_list*) ;
void writeStringList(string_list*);

int readShort(void) ;
void writeShort(int);

int readInt(void) ;
void writeInt(int);

float readFloat(void) ;
void writeFloat( float );

double readDouble(void) ;
void writeDouble( double );

storage* writePacket(unsigned char*, int);

//void writeStorage(storage & );
void freeStorage(storage *);

#endif

