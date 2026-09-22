/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief definition of configuration parameters for the NVIDIA Aerial L1 integration
 */

#ifndef __GNB_APP_AERIAL_PARAMDEF__H__
#define __GNB_APP_AERIAL_PARAMDEF__H__

/* Aerial configuration section name */
#define CONFIG_STRING_AERIAL "Aerial"

/* Aerial configuration parameters names */
#define AERIAL_NUM_TX_ANT "num_tx_ant"
#define AERIAL_NUM_RX_ANT "num_rx_ant"

#define HLP_AERIAL_NUM_TX_ANT \
  "Physical TX antennas advertised to the Aerial L1 in CONFIG.request (numTxAnt). 0 = derive from logical antenna ports"
#define HLP_AERIAL_NUM_RX_ANT \
  "Physical RX antennas advertised to the Aerial L1 in CONFIG.request (numRxAnt), sizes the L1 UL receive processing. 0 = derive from logical antenna ports"

/*-----------------------------------------------------------------------------------------------------------------------*/
/*                                            Aerial configuration parameters                                             */
/*   optname             helpstr                paramflags    XXXptr        defXXXval    type      numelt                 */
/*-----------------------------------------------------------------------------------------------------------------------*/
// clang-format off
#define AERIALPARAMS_DESC { \
  {AERIAL_NUM_TX_ANT,    HLP_AERIAL_NUM_TX_ANT, 0,            .iptr=NULL,   .defintval=0, TYPE_INT, 0}, \
  {AERIAL_NUM_RX_ANT,    HLP_AERIAL_NUM_RX_ANT, 0,            .iptr=NULL,   .defintval=0, TYPE_INT, 0}, \
}
// clang-format on

/*-----------------------------------------------------------------------------------------------------------------------*/
#endif
