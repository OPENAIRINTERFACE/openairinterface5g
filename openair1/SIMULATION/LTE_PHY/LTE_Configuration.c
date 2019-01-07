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

#include "LTE_Configuration.h"


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


void calibration(unsigned int gpib_card,unsigned int gpib_device,unsigned int freqband,LTE_DL_FRAME_PARMS *frame_parms,DCI_ALLOC_t *dci_alloc)
{
  char string[256];
  void *DLSCH_alloc_pdu = (void *)dci_alloc[0].dci_pdu;

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

  gpib_send(gpib_card,gpib_device,"BB:EUTR:LINK DOWN");
  //gpib_send(gpib_card,gpib_device,"BB:EUTR:SETT:TMOD:DL 'E-TM1_1__20MHz'");  //EUTRA test model
  gpib_send(gpib_card,gpib_device,"BB:EUTR:SLEN 4");                        //sequence length of the signal in number of frames



  /*
  //Filter/Clipping/Power Settings
  gpib_send(gpib_card,gpib_device,"BB:EUTR:FILT:TYPE LTEF");              //set the filter type into LET filter
  gpib_send(gpib_card,gpib_device,"BB:EUTR:FILT:MODE RT");                //select a realtime filter mode
  gpib_send(gpib_card,gpib_device,"BB:EUTR:FILT:PAR:LTE:COFF 0.35");     // set the roll-off factor for LTE filter
  gpib_send(gpib_card,gpib_device,"BB:EUTR:FILT:PAR:LTE:COFS -0.3");     //set the cut of the frequency for LET filter, here -0.3 the BW is 20 MHz
  gpib_send(gpib_card,gpib_device,"BB:EUTR:FILT:PAR:LTE:OPT ACPN");      //selects ACP (Narrow) optimization
  gpib_send(gpib_card,gpib_device,"BB:EUTR:FILT:PAR:LTE:ROF 0.2");       //the roll-off factor is set to 0.2.



  // the clock setting
  gpib_send(gpib_card,gpib_device,"BB:EUTR:CLOC:SOUR EXT");          //Selects the clock source
  gpib_send(gpib_card,gpib_device,"BB:EUTR:CLOC:MODE SAMP");         //selects clock type SAMPle
  gpib_send(gpib_card,gpib_device,"BB:EUTR:CLOC:SYNC:MODE MAST");   //the instrument is configured to work as a master one.
  gpib_send(gpib_card,gpib_device,"BB:EUTR:CLOCK:SYNC:EXEC");       //all synchronization's settings are adjusted accordingly

  */




  // General EUTRA/LTE Settings
  sprintf(string,"BB:EUTR:TDD:UDC %d",frame_parms->tdd_config);
  gpib_send(gpib_card,gpib_device,string);  //sets the UL/DL configuration into 3
  sprintf(string,"BB:EUTR:TDD:SPSC %d",frame_parms->tdd_config_S);
  gpib_send(gpib_card,gpib_device,string); //sets the special subframe configuration into 0






  //General EUTRA/LTE Downlink Settings
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:CONF:MODE AUTO");  //Determines whether the configuration and scheduling of the different PDSCH allocations
  //inside of the DL allocation table is performed manual or according to the configuration
  //made for the DCIs
  //sets the MIMO configuration to 1 TxAntenna
  sprintf(string,"BB:EUTR:DL:MIMO:CONF TX%d",frame_parms->nb_antennas_tx);
  gpib_send(gpib_card,gpib_device,string);

  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:MIMO:ANTA ANT1"); //Sets the simulated antenna for path A, selesct antenna 1
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:MIMO:ANTB NONE"); //Set the Simulation antenna for path B, we Just have one antenna, so for path B, no antenna
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SYNC:TXAN ALL");  //enables all antennas port to transmit P-/S-SYNC
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:BW USER");       //selects a downlink frequency band of USER

  //sets the number of resource blocks to 25
  sprintf(string,"BB:EUTR:DL:NORB %d",frame_parms->N_RB_DL);
  gpib_send(gpib_card,gpib_device,string);

  //size of FFT
  //sprintf(string,"BB:EUTR:DL:FFT %d",frame_parms->ofdm_symbol_size); //depends on the number of resource block
  //gpib_send(gpib_card,gpib_device,string);

  //sets the Cell ID 0
  sprintf(string,"BB:EUTR:DL:PLC:CID %d",frame_parms->Nid_cell);
  gpib_send(gpib_card,gpib_device,string);


  //gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:PLC:PLID 2");  //physical layer is 2

  //set the prefix of the subframes
  if (frame_parms->Ncp == 0)
    gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:CPC NORM");
  else
    gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:CPC EXT");


  //DL Frame Configuration
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:VIEW:MODE PRB");  //Determines the time plan shows the allocated Physical Resource Blocks
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:BUR DUD");         //the unscheduled resource elements are filled with dummy data.
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SFS 6");          //displays subframe 6 in the allocation list view.
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ALC 1");

  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ALL1:CW:AOC ON");
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ALL1:CW:CONT PDSC"); //selects PDSCH as type for the selected allocation
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ALL1:CW1:MOD QPSK");//choose modulation QPSK
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF4:ALL1:CW:DATA USER1"); // user1 is the data source for the selected allocation
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ALL1:CW:STAT ON"); //activates the selected allocation.

  //PDCCH & PSCH  configuration
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:STAT ON");  //enables PDCCH allocation
  //gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PREC:SCH TXD");
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:SCR:STAT ON"); //enables scrambling
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:FORM VAR");     //sets the PDCCH format into VAR
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:RES"); //reset the table table
  //gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:SIT 0"); //selects the first row in the DCI table
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:APP");


  //sets the n_RNTI into 1234
  sprintf(string,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:UEID %d",dci_alloc->rnti);
  gpib_send(gpib_card,gpib_device,string);

  sprintf(string,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:UEID %d",dci_alloc->rnti);
  gpib_send(gpib_card,gpib_device,string);

  //sets the DCI format into 1
  sprintf(string,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIF F%d",dci_alloc->format);
  gpib_send(gpib_card,gpib_device,string);

  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:PFMT 0");     //Sets the PDCCH format for the selected PDCCH into 0

  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:F1AM PDSC");//Selects the mode of the DCI format

  //gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF1:ENCC:PDCC:EXTC:USER USER1");     //the DCI is dedicated to User 1

  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:SESP OFF"); //the DCI is transmitted within the common or UE-specific search OFF


  if(dci_alloc->format==format1) {
    //sets the HARQ process number
    sprintf(string,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:HPN %d",((DCI1_5MHz_TDD_t*) DLSCH_alloc_pdu)->harq_pid);
    gpib_send(gpib_card,gpib_device,string);

    //sets the Modulation and Coding Scheme
    sprintf(string,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:MCSR %d",((DCI1_5MHz_TDD_t*)DLSCH_alloc_pdu)->mcs);
    gpib_send(gpib_card,gpib_device,string);

    //sets the New Data Indicator
    if(((DCI1_5MHz_TDD_t*)DLSCH_alloc_pdu)->ndi==1)
      gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:NDI ON");
    else
      gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:NDI OFF");

    //disables Resource Allocation Header
    if(((DCI1_5MHz_TDD_t*)DLSCH_alloc_pdu)->rah==1)
      gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:RAH ON");
    else
      gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:RAH OFF");

    //sets Resource Block Assignment
    sprintf(string,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:RBA %d",((DCI1_5MHz_TDD_t*)DLSCH_alloc_pdu)->rballoc );
    gpib_send(gpib_card,gpib_device,string);

    //sets the Redundancy Version
    sprintf(string,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:RV %d",((DCI1_5MHz_TDD_t*)DLSCH_alloc_pdu)->rv );
    gpib_send(gpib_card,gpib_device,string);

    //sets the TPC Command for PUSCH
    sprintf(string,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:ITEM1:DCIC:TPCC %d",((DCI1_5MHz_TDD_t*)DLSCH_alloc_pdu)->TPC );
    gpib_send(gpib_card,gpib_device,string);
  }

  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:SIT 0");
  gpib_send(gpib_card,gpib_device,"BB:EUTR:DL:SUBF6:ENCC:PDCC:EXTC:DEL");
  //gpib_send(gpib_card,gpib_device,"AWGN:STAT ON"); // to add noise in the system
  gpib_send(gpib_card,gpib_device,"FREQ 1.2GHz");
  gpib_send(gpib_card,gpib_device,"SOUR:POW:POW -50");
  gpib_send(gpib_card,gpib_device,"SYSTem:ERRor?");
  gpib_send(gpib_card,gpib_device,"OUTP ON");


}



