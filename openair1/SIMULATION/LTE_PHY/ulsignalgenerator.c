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
#include<stdio.h>
#include<string.h>
#include<gpib/ib.h>

#include "ulsignalgenerator.h"


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

void puschsignalG(unsigned int gpib_card,unsigned int gpib_device,unsigned int freqband,LTE_DL_FRAME_PARMS *frame_parms,void *UL_alloc_pdu)
{
  char string[256];
  //void *UL_alloc_pdu = (void *)dci_alloc[0].dci_pdu;

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

  //UL Frame Configuration pusch

  gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:CONS 5");//3 UL subframes of a frame are configurable

  //gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:UET UE1");

  //gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:STAT ON"); //Sets the allocation state to active

  //gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:CONT PUCS");

  //gpib_send(gpib_card,gpib_device,"BB:EUTR:UL:SUBF2:ALL0:MOD QAM16");



  gpib_send(gpib_card,gpib_device,"FREQ 1.2GHz");
  gpib_send(gpib_card,gpib_device,"SOUR:POW:POW -50");
  gpib_send(gpib_card,gpib_device,"SYSTem:ERRor?");
  gpib_send(gpib_card,gpib_device,"OUTP ON");




}
