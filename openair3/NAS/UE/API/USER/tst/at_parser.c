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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
 Source   main.c
 Version  0.1
 Date   01/04/2012
 Product  NAS stack
 Subsystem  Application Programming Interface
 Author   Baris Demiray
 Description  Interacts with AT parser for testing purposes
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "at_command.h"

#include "nas_log.h"

int main()
{
  const char * sample_at_commands[] = {
    "AT+CGSN\r", "AT+CGSN=?\r", "AT+CGMI\r", "AT+CGMI=?\r", "AT+CGMM\r",
    "AT+CGMM=?\r", "AT+CIMI\r", "AT+CIMI=?\r", "AT+CFUN=1,1\r",
    "AT+CFUN=2\r", "AT+CFUN?\r", "AT+CFUN=?\r", "AT+CPIN=1234\r",
    "AT+CPIN=1234,4321\r", "AT+CPIN?\r", "AT+CPIN=?\r", "AT+CSQ\r",
    "AT+CSQ=?\r", "AT+COPS=1,1,EURECOM\r", "AT+COPS=2,3\r", "AT+COPS=5\r",
    "AT+COPS?\r", "AT+COPS=?\r", "AT+CGREG=1\r", "AT+CGREG?\r",
    "AT+CGREG=?\r", "AT+CGDCONT=1\r", "AT+CGDCONT?\r", "AT+CGDCONT=?\r",
    "AT+CGACT=1\r", "AT+CGACT?\r", "AT+CGACT=?\r", "AT+CGMR\r",
    "AT+CGMR=?\r", "AT+CESQ\r", "AT+CESQ=?\r", "AT+CMEE=2\r",
    "AT+CMEE?\r", "AT+CMEE=?\r", "AT+CLCK=SC,2\r", "AT+CLCK=?\r",
    "AT+CREG=1\r", "AT+CREG?\r", "AT+CREG=?\r",
    "AT+CEREG=1\r", "AT+CEREG?\r", "AT+CEREG=?\r",
    "AT+CGATT=1\r", "AT+CGATT?\r", "AT+CGATT=?\r",
    "AT\r", "null"
  };

  const char * sample_faulty_at_commands[] = {
    "AT+CGSN=3,4\r", "AT+CGSN=??\r", "AT+CGMI=1,b\r", "AT+CGMI==\r",
    "AT+CGMM?\r", "AT+CGMM==n\r", "AT+CIMI=4\r", "AT+CIMI=?t\r",
    "AT+CFUN=n1,1\r", "AT+CFUN??\r", "AT+CFUN=a?\r", "AT+CPIN=1fd4\r",
    "AT+CPIN=1sd+,432\r", "AT+CPIN?4\r", "AT+CPIN=?,?\r", "AT+CSQ=AT\r",
    "AT+CSQ=q\r",
    "AT+COPS=123121123123123123,EURECOMEURECOMEURECOMEURECOM\r",
    "AT+COPS=2asd3,33ads1\r", "AT+COPS.=5\r", "AT+COPS?/r\\r",
    "AT+COPS=?\n\r\r", "AT+CGREG=da1243\42r", "AT+CGREG.?\r",
    "AT+CGREG1=?\r", "AT+CGDCONT=d31\r", "AT+CGDCONTxc?\\crca",
    "AT+CGDCONcTc=c?\r", "AT+CGA12C3T=11\123r", "AT+CGACT';;.,?\r",
    "AT+CGACT=?]\\;e23\r", "\r\r\r", "AT+COP\r", "null"
  };

  at_command_t at_command;
  unsigned int at_command_index;

  //log_init(0x2f);

  /* Parse valid AT commands */
  at_command_index = 0;

  while (strncmp(sample_at_commands[at_command_index], "null", 4) != 0) {
    if (at_command_decode(sample_at_commands[at_command_index],
                          strlen(sample_at_commands[at_command_index]),
                          &at_command) > 0) {
      printf("INFO: Fine, parser interpreted valid command %s\n",
             sample_at_commands[at_command_index]);
    } else {
      printf("ERROR: Parser rejected valid command %s\n",
             sample_at_commands[at_command_index]);
    }

    at_command_index++;
  }

  /* Parse invalid AT commands */
  at_command_index = 0;

  while (strncmp(sample_faulty_at_commands[at_command_index], "null", 4) != 0) {
    if (at_command_decode(sample_faulty_at_commands[at_command_index],
                          strlen(sample_faulty_at_commands[at_command_index]),
                          &at_command) > 0) {
      printf("ERROR: Parser interpreted invalid command %s\n",
             sample_faulty_at_commands[at_command_index]);
    } else {
      printf("INFO: Fine, parser rejected the command %s\n",
             sample_faulty_at_commands[at_command_index]);
    }

    at_command_index++;
  }

  /*
   * Provide a console to the user
   */
  int bytes_read;
  size_t input_size = 100;
  char *input;

  printf("Note 1: Carriage return will be appended to every command by default\n");
  printf("Note 2: Enter 'quit' to exit\n");
  printf("AT Command> ");

  input = (char *)malloc(input_size + 1);

  bytes_read = getline(&input, &input_size, stdin);

  while (strncmp(input, "quit", 4) != 0) {
    if (bytes_read == -1) {
      puts("Cannot read user input!\n");
    } else {
      input[bytes_read - 1] = '\r';

      if (at_command_decode(input, bytes_read, &at_command) > 0)
        printf("INFO: Parser interpreted command %s\n", input);
      else
        printf("ERROR: Parser rejected command %s\n", input);
    }

    printf("\nAT Command> ");
    memset(input, 0x00, input_size);
    bytes_read = getline(&input, &input_size, stdin);
  }

  free(input);

  printf("Exiting...\n");

  return 0;
}
