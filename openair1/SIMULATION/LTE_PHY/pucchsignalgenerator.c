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

#include "pucchsignalgegerator.h"


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


void pucchsignalG(unsigned int gpib_card,unsigned int gpib_device,unsigned int freqband,LTE_DL_FRAME_PARMS *frame_parms,PUCCH_FMT_t pucch_format)
{
  char string[256];
  //void *DLSCH_alloc_pdu = (void *)dci_alloc[0].dci_pdu;

  //Start the configuration
  gpib_send(gpib_card,gpib_device,"*RST;*CLS");   //reset and configure the signal generator
  gpib_send(gpib_card,gpib_device,"BB:EUTR:PRES");
  gpib_send(gpib_card,gpib_device,"BB:EUTR:STAT ON");

  gpib_send(gpib_card,gpib_device,"POW -70dBm");  // set output signal power
  //gpib_send(gpib_card,gpib_device,"FREQ 1.91");  // set frequency

  //Selects the duplexing mode
  if (frame_parms->frame_type == 0)
    gpib_send(gpib_card,gpib_device,"BB:EUTR:DUPL FDD");
  else
    gpib_send(gpib_card,gpib_device,"BB:EUTR:DUPL TDD");

  gpib_send(gpib_card,gpib_device,"BB:EUTR:LINK UP");
  gpib_send(gpib_card,gpib_device,"BB:EUTR:SLEN 4");                        //sequence length of the signal in number of frames

  // General EUTRA/LTE Settings
  sprintf(string,"BB:EUTR:TDD:UDC %d",frame_parms->tdd_config);
  gpib_send(gpib_card,gpib_device,string);  //sets the UL/DL configuration into 3
  sprintf(string,"BB:EUTR:TDD:SPSC %d",frame_parms->tdd_config_S);
  gpib_send(gpib_card,gpib_device,string); //sets the special subframe configuration into 0


  //General EUTRA/LTE UPlink Settings

  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:BW USER"); //set the bandwidth into 5 MHz ??

  sprintf(string,"BB:EUTR:UL:NORB %d",frame_parms->N_RB_UL); //sets the number of resource blocks to 25
  gpib_send(gpib_card,gpib_device,string);

  sprintf(string,"BB:EUTR:UL:PLC:CID %d",frame_parms->Nid_cell); //sets the Cell ID 0
  gpib_send(gpib_card,gpib_device,string);


  if (frame_parms->Ncp == 0)
    gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:CPC NORM"); //set the prefix of the subframes
  else
    gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:CPC EXT");

  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:REFS:GRPH OFF"); //disables group hopping

  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:REFS:SEQH OFF"); //disables sequence hopping

  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:REFS:SRS:ANST OFF"); //disables the A/N ans SRS simultaneous transmission for UE2





  //UL Frame Configuration pucch
  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:CONS 10");//10 UL subframes of a frame are configurable

  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:STAT ON"); //Sets the allocation state to active

  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:CONT PUCC");

  //pucch format
  if(pucch_format==pucch_format1) {
    gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:FORM F1");
  }

  if(pucch_format==pucch_format1a) {
    gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:FORM F1A");
  }

  if(pucch_format==pucch_format1b) {
    gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:FORM F1B");
  }

  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:VRB 2"); //sets the VRB Offset

  //RF signal setting
  gpib_send(gpib_card,gpib_device,"FREQ 1.2GHz");
  gpib_send(gpib_card,gpib_device,"SOUR:POW:POW -50");
  gpib_send(gpib_card,gpib_device,"SYSTem:ERRor?");
  gpib_send(gpib_card,gpib_device,"OUTP ON");


}




















