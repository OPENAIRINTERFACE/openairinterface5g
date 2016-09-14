/*! \file extern.h
 *  \brief specifies the extern variables for phy emulation
 *  \author Navid Nikaein and Raymomd Knopp and Hicham Anouar
 *  \date 2011
 *  \version 1.1
 *  \company Eurecom
 *  \email: navid.nikaein@eurecom.fr
 */

#ifndef __BYPASS_SESSION_LAYER_EXTERN_H__
#    define __BYPASS_SESSION_LAYER_EXTERN_H__

#ifndef USER_MODE
#include <rtai_posix.h>
#else
#include <pthread.h>
#endif //RTAI_ENABLED

extern unsigned char Emulation_status;
extern unsigned char emu_tx_status;
extern unsigned char emu_rx_status;
//extern unsigned int Master_list;
//extern unsigned short Master_id;
//extern unsigned int Is_primary_master;

#if !defined(ENABLE_NEW_MULTICAST)
extern pthread_mutex_t emul_low_mutex;
extern pthread_cond_t emul_low_cond;
extern char emul_low_mutex_var;
extern pthread_mutex_t Tx_mutex;
extern pthread_cond_t Tx_cond;
extern char Tx_mutex_var;
#endif

extern rx_handler_t rx_handler;
extern tx_handler_t tx_handler;

extern eNB_transport_info_t eNB_transport_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs];
extern uint16_t eNB_transport_info_TB_index[NUMBER_OF_eNB_MAX][MAX_NUM_CCs];

extern UE_transport_info_t UE_transport_info[NUMBER_OF_UE_MAX][MAX_NUM_CCs];
extern uint16_t UE_transport_info_TB_index[NUMBER_OF_UE_MAX][MAX_NUM_CCs];

extern UE_cntl ue_cntl_delay[NUMBER_OF_UE_MAX][MAX_NUM_CCs][2];

#endif
