#include "PHY/types.h"
#include "PHY/defs.h"

void gpib_send(unsigned int gpib_board, unsigned int gpib_device, char *command_string );
void calibration(unsigned int gpib_card,unsigned int gpib_device,unsigned int freqband,LTE_DL_FRAME_PARMS *frame_parms,DCI_ALLOC_t *dci_alloc);
