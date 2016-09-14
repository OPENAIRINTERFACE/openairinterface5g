#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"


void bit8_txmux(int length,int offset)
{

  int i;
  short  *dest,*dest2;




  for (i=0; i<length; i++) {

    dest = (short *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i+offset];
    dest2 =   (short *)&PHY_vars->tx_vars[1].TX_DMA_BUFFER[i+offset];

    ((char *)dest)[0] = (char)(dest[0]>>BIT8_TX_SHIFT);
    ((char *)dest)[1] = (char)(dest[1]>>BIT8_TX_SHIFT);
    ((char *)dest)[2] = (char)(dest2[0]>>BIT8_TX_SHIFT);
    ((char *)dest)[3] = (char)(dest2[1]>>BIT8_TX_SHIFT);
  }


}
