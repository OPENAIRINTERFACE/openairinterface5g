#include<stdio.h>
#include<string.h>
#include<gpib/ib.h>
#include"gpib_send.h"
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

