/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
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

#include <stdio.h>
#include <string.h>
#include <gpib/ib.h>
#include "gpib_send.h"
void gpib_send(unsigned int gpib_board, unsigned int gpib_device, char *command_string )
{
  unsigned short addlist[2] = {gpib_device, NOADDR};
  SendIFC(gpib_board);

  //Enable all on GPIB bus
  EnableRemote(gpib_board, addlist);


  if(ibsta & ERR) {
    printf("gpib_send: Instrument enable failed! \n");
  }

  //Send Control Commandss
  Send(gpib_board, gpib_device, command_string, strlen(command_string), NLend);

  if(ibsta & ERR) {

    printf("gpib_send: Send failed! \n");

  }

  printf("%s \n",command_string);

}

