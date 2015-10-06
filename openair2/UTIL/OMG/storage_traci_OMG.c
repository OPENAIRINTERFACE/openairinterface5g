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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file storage_traci_OMG.c
* \brief The data storage object carrying data from/to SUMO. 'C' reimplementation of the TraCI version of simITS (F. Hrizi, fatma.hrizi@eurecom.fr)
* \author  S. Uppoor
* \date 2012
* \version 0.2
* \company INRIA
* \email: sandesh.uppor@inria.fr
* \note 0.1 -> 0.2: (J. Harri) added reader and writer to String_list
* \warning
*/


#include <string.h>
#include "storage_traci_OMG.h"

//using namespace std
bool bigEndian;


void reset()
{
  tracker = NULL;
  head = NULL;
  descLen = 0;
  freeStorage(storageStart);  // freeing the storage object
  storageStart=NULL;

}

void check_endianness()
{

  //checking endianness
  un.s = 0x0102;

  if (sizeof(short) == 2) {
    if (un.c[0] == 1 && un.c[1] == 2)
      bigEndian = true;
  }
}


int storageLength(storage *cur_ptr)
{

  int count=0;

  while(cur_ptr != NULL) {
    cur_ptr=cur_ptr->next;
    count++;
  }

  return(count);
}

void rearange()
{
  //Since linked list is used, all data is appended to the last pointer,
  // we face problem in sendExact(int cmdLength) in socket_traci_OMG.c
  // since total length of the entire message is appended last
  // storage = command + length(4 bytes) is changed to storage = length(4 bytes) + command
  int pos = msgLength - 4;
  int count =0;

  storage *cur_ptr = NULL;
  storage *temp_ptr = NULL;
  storage *temp_pptr = NULL;

  cur_ptr = storageStart;

  while (count != pos && cur_ptr->next != NULL) {
    temp_pptr = cur_ptr;
    cur_ptr = cur_ptr->next;

    count++;
  }

  temp_ptr = cur_ptr;

  while(cur_ptr->next != NULL) {
    cur_ptr = cur_ptr-> next;
  }

  cur_ptr->next = storageStart;
  storageStart = temp_ptr;
  temp_pptr->next = NULL;

}

unsigned char readChar()
{
  // if (tracker!=NULL)
  //  {
  unsigned char hd = tracker->item;
  //printf("OMG char: %d \n",(int)hd);
  tracker = tracker->next;
  return hd;
  //}else
  //  {
  //       LOG_E(OMG, " Storage::readChar():  Tracker is NULL \n");
  //       unsigned char *error = NULL;
  //       return error;  //
  //  }


}

void writeChar(unsigned char value)
{
  storage *temp;
  temp = (storage *)malloc(sizeof(storage));

  if (head == NULL) {
    head = temp;
    head->next = NULL;
    head->item = value;
    storageStart = temp; //points to start of actual message
  } else {
    head->next = temp;
    temp->next = NULL;
    temp->item = value;
    head = temp;
  }
}

int readByte()
{
  int i = (int)(readChar());

  if (i < 128) return i;
  else return (i - 256);

}

void writeByte(int value)
{
  if (value < -128 || value > 127) {
    LOG_E(OMG, " Storage::writeByte(): Invalid value, not in [-128, 127]\n");
  }

  writeChar( (unsigned char)( (value+256) % 256 ) );
}

int readUnsignedByte()
{

  return (int)(readChar());

}


void writeUnsignedByte(int value)
{
  if (value < 0 || value > 255) {
    LOG_E(OMG, " Storage::writeUnsignedByte(): Invalid value, not in [0, 255]\n");
  }

  writeChar( ((unsigned char)value));
}

// JNNOTE: had to change the behavior here, as tmp was always pointing at the end of the String and not at the beginning: accordingly, all strings where always '0'
char * readString()
{

  int i=0;
  int len = readInt();

  descLen = len;
  char *tmp = (char *)malloc(sizeof(char) * (len));
  char *ret = tmp; // JHNOTE: added a pointer pointing at the head of the String

  //printf("OMG ready to readString of length %d \n",len);
  for (; i < len; i++) {
    *tmp++ = (char) readChar();
  }

  *tmp++ = '\0'; // makes sure it's NUL terminated

  //printf("OMG readString - %s \n",ret);
  //printf("OMG readString - %s \n",tmp);
  return ret; // the head is returned

}


void writeString(char *s)
{
  int count=0;
  char* s_copy=s;

  for(; *s!='\0'; s++)
    count++;

  writeInt(count);

  for(; *s_copy!='\0'; s_copy++)
    writeChar(*s_copy);

}

string_list* readStringList(string_list* vector)
{

  int i=0;
  int len = readInt();
  descLen = len;
  string_list* entry = NULL;

  for (; i < len; i++) {
    if (vector->string == NULL) {
      char *tmp = readString();
      //printf("OMG - 1 SUMO ID: %s \n",tmp);
      vector->string = tmp;//readString();
    } else {
      entry = (string_list*) malloc(sizeof(string_list));
      char *tmp = readString();
      //printf("OMG - SUMO ID: %s \n",tmp);
      entry->string = tmp;//readString();
      entry->next = NULL;

      if(vector !=NULL) {
        string_list* tmp = vector;

        while (tmp->next != NULL) {
          tmp = tmp->next;
        }

        tmp->next = entry;
      }
    }
  }

  return vector;

}


void writeStringList(string_list* vector)
{


  int count=0;
  string_list* tmp = vector;

  // JHNote: a better solution would be to save the pointer to the reference of this Int and replace the value with count at the end....
  if (tmp->string !=NULL)
    count++;

  while (tmp->next != NULL) {
    tmp = tmp->next;
    count++;
  }

  writeInt(count);

  tmp = vector;

  if (tmp->string !=NULL) {
    writeString(tmp->string);
  }

  while (tmp->next != NULL) {
    tmp = tmp->next;
    writeString(tmp->string);
  }

}


int readShort()
{
  LOG_W(OMG, " readShort not implented \n");
  return -1;
}

void writeShort(int value)
{
  LOG_W(OMG, " writeShort not implented \n");
  return;
}


int readInt()
{
  int value = 0;
  unsigned char *p_value = (unsigned char*)(&value);

  if (bigEndian) {
    // network is big endian
    p_value[0] = readChar();
    p_value[1] = readChar();
    p_value[2] = readChar();
    p_value[3] = readChar();
  } else {
    // network is little endian
    p_value[3] = readChar();
    p_value[2] = readChar();
    p_value[1] = readChar();
    p_value[0] = readChar();
  }

  return value;

}


void writeInt(int value)
{
  unsigned char *p_value = (unsigned char*)(&value);

  if (bigEndian) {
    // network is big endian
    writeChar(p_value[0]);
    writeChar(p_value[1]);
    writeChar(p_value[2]);
    writeChar(p_value[3]);

  } else {
    // network is little endian

    writeChar(p_value[3]);
    writeChar(p_value[2]);
    writeChar(p_value[1]);
    writeChar(p_value[0]);
  }
}

float readFloat()
{

  float value = 0;
  unsigned char *p_value = (unsigned char*)(&value);

  if (bigEndian) {
    // network is big endian
    p_value[0] = readChar();
    p_value[1] = readChar();
    p_value[2] = readChar();
    p_value[3] = readChar();
  } else {
    // network is big endian
    p_value[3] = readChar();
    p_value[2] = readChar();
    p_value[1] = readChar();
    p_value[0] = readChar();
  }

  return value;

}


void writeFloat(float value)
{

  unsigned char *p_value = (unsigned char*)(&value);

  if (bigEndian) {
    // network is big endian
    writeChar(p_value[0]);
    writeChar(p_value[1]);
    writeChar(p_value[2]);
    writeChar(p_value[3]);
  } else {
    // network is big endian
    writeChar(p_value[3]);
    writeChar(p_value[2]);
    writeChar(p_value[1]);
    writeChar(p_value[0]);
  }
}


double readDouble()
{

  double value = 0;
  unsigned char *p_value = (unsigned char*)(&value);

  if (bigEndian) {
    // network is big endian
    int i = 0;

    for (; i<8; ++i) {
      p_value[i] = readChar();
    }
  } else {
    int i=7;

    // network is big endian
    for (; i>=0; --i) {
      p_value[i] = readChar();
    }
  }

  return value;

}

void writeDouble(double value)
{
  unsigned char *p_value = (unsigned char*)(&value);

  if (bigEndian) {
    int i=0;

    // network is big endian
    for (; i<8; ++i) {
      writeChar(p_value[i]);
    }
  } else {
    int i=7;

    // network is big endian
    for (; i>=0; --i) {
      writeChar(p_value[i]);
    }
  }

}



storage* writePacket (unsigned char* packet, int length)
{

  //printf("required length is %d \n",length);

  storage *recvpacket = NULL;
  storage *recvpacketStart= NULL;
  storage *temp_  = NULL;

  int localSize = sizeof(storage);
  printf("size of storage is %d bytes \n",localSize);

  int i = 0;

  for(; i < length; i++) {

    printf("in %d round \n",i);
    //storage *temp_ = (storage *)malloc(sizeof(storage));
    temp_ = (storage *)malloc(sizeof(storage));


    if  (recvpacket == NULL) {
      recvpacket = temp_;
      recvpacketStart = recvpacket;
      recvpacket->next = NULL;
      recvpacket->item = packet[i];
    } else {
      temp_->next =NULL;
      temp_-> item = packet[i];
      recvpacket-> next = temp_;
      recvpacket =temp_;
    }
  }

  return recvpacketStart;

}


/*void writeStorage(){
  LOG_W(OMG, " writeStorage not implented \n");
  return;

}*/

void freeStorage(storage * freePtr)
{

  storage *node,*temp;
  node = freePtr;          // start at the head.

  while (node != NULL) {   // traverse entire list.
    temp = node ;        // save node pointer.
    node = node->next;   // advance to next.
    free(temp);
  }            // free the saved one.

  freePtr = NULL;             // finally, mark as empty list.
}

void reset_String_list(string_list* vector)
{
  string_list* entry = vector;
  string_list* tmp;

  while (entry->next != NULL) {
    tmp = entry;
    entry = entry->next;
    free(tmp);
  }

  vector = NULL;
}


