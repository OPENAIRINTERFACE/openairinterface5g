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

#include<stdio.h>
#include<string.h>
#include<gpib/ib.h>
#include "signalanalyzer.h"

void gpib_send(unsigned int gpib_card, unsigned int gpib_device, char *command_string )
{
  unsigned short addlist[2] = {gpib_device, NOADDR};
  SendIFC(gpib_card);

  //Enable all on GPIB bus
  EnableRemote(gpib_card, addlist);


  if(ibsta & ERR) {
    printf("gpib_send: Instrument enable failed! \n");
  }

  //Send Control Commandss
  Send(gpib_card, gpib_device, command_string, strlen(command_string), NLend);

  if(ibsta & ERR) {

    printf("gpib_send: Send failed! \n");

  }

  printf("%s \n",command_string);

}


void analyzer(unsigned int gpib_card,unsigned int gpib_device,unsigned int freqband,LTE_DL_FRAME_PARMS *frame_parms,DCI_ALLOC_t *dci_alloc)
{
  char string[256];

  //Start the remote control
  gpib_send(gpib_card,gpib_device,"*RST;*CLS");   //reset and configure the signal analyzer
  gpib_send(gpib_card,gpib_device,"CONF:PRES");


  gpib_send(gpib_card,gpib_device,"CALC1:FEED 'CONS:CONS'");
  gpib_send(gpib_card,gpib_device,"INIT");


  //select the duplexing mode
  if (frame_parms->frame_type == 0)
    gpib_send(gpib_card,gpib_device,"CONF:DUPL FDD");
  else
    gpib_send(gpib_card,gpib_device,"CONF:DUPL TDD");

  //set the direction into DL
  gpib_send(gpib_card,gpib_device,"CONF:LDIR DL");

  //carrier frequency
  gpib_send(gpib_card,gpib_device,"FREQ:CENT 1.2GHZ");

  //input source
  gpib_send(gpib_card,gpib_device,"INP RF");

  //RF attenuation
  gpib_send(gpib_card,gpib_device,"INP:ATT 0");

  ////Using a Trigger
  gpib_send(gpib_card,gpib_device,"TRIG:MODE IMM"); //Selects free run trigger source

  // number of frames to be selected
  gpib_send(gpib_card,gpib_device,"FRAM:COUN:STAT ON");
  gpib_send(gpib_card,gpib_device,"FRAM:COUN 4");

  //select all the subframes
  gpib_send(gpib_card,gpib_device,"SUBF:SEL ALL");

  //Identifies the configuration according to the data in the PDCCH DCIs
  gpib_send(gpib_card,gpib_device,"DL:FORM:PSCD PDCCH");

  //sets the number of resource blocks to 25
  sprintf(string,"CONF:DL:NORB %d",frame_parms->N_RB_DL);
  gpib_send(gpib_card,gpib_device,string);


  //set the prefix of the subframes
  if (frame_parms->Ncp == 0)
    gpib_send(gpib_card,gpib_device,"CONF:DL:CYCP NORM");
  else
    gpib_send(gpib_card,gpib_device,"CONF:DL:CYCP EXT");

  //select the UL/DL subframe configuration for downlink signals
  sprintf(string,"CONF:DL:TDD:UDC %d",frame_parms->tdd_config);
  gpib_send(gpib_card,gpib_device,string);  //sets the UL/DL configuration into 3

  //Selects the configuration of a TDD special subframe
  sprintf(string,"CONF:DL:TDD:UDC %d",frame_parms->tdd_config_S);
  gpib_send(gpib_card,gpib_device,string); //sets the special subframe configuration into 0


}
