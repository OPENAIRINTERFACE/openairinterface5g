/*******************************************************************************

 *******************************************************************************/
/*! \file PHY/LTE_REFSIG/defs_NB_IoT.c
* \function called by lte_dl_cell_spec_NB_IoT.c ,	 TS 36-211, V13.4.0 2017-02
* \author M. KANJ
* \date 2017
* \version 0.0
* \company bcom
* \email: matthieu.kanj@b-com.com
* \note
* \warning
*/

/* Definitions for NB_IoT Reference signals */

#ifndef __LTE_REFSIG_DEFS_NB_IOT__H__
#define __LTE_REFSIG_DEFS_NB_IOT__H__

#include "PHY/defs_L1_NB_IoT.h"

/** @ingroup _PHY_REF_SIG
 * @{
*/
/*!\brief This function generates the LTE Gold sequence (36-211, Sec 7.2), specifically for DL reference signals.
@param frame_parms LTE DL Frame parameters
@param lte_gold_table pointer to table where sequences are stored
@param Nid_cell Cell Id for NB_IoT (to compute sequences for local and adjacent cells) */

void lte_gold_NB_IoT(NB_IoT_DL_FRAME_PARMS  *frame_parms,
					 uint32_t 				lte_gold_table_NB_IoT[20][2][14],
					 uint16_t 				Nid_cell);

/*! \brief This function generates the Narrowband reference signal (NRS) sequence (36-211, Sec 6.10.1.1)
@param phy_vars_eNB Pointer to eNB variables
@param output Output vector for OFDM symbol (Frequency Domain)
@param amp Q15 amplitude
@param Ns Slot number (0..19)
@param l symbol (0,1) - Note 1 means 3!
@param p antenna index
@param RB_IoT_ID the ID of the RB dedicated for NB_IoT
*/
int lte_dl_cell_spec_NB_IoT(PHY_VARS_eNB_NB_IoT  *phy_vars_eNB,
                     		int32_t 			 *output,
                     		short 				 amp,
                     		unsigned char 		 Ns,
                     		unsigned char 		 l,
                     		unsigned char 		 p,
					 		unsigned short 		 RB_IoT_ID); 


unsigned int lte_gold_generic_NB_IoT(unsigned int  *x1,
									 unsigned int  *x2,
									 unsigned char reset);
		
void generate_ul_ref_sigs_rx_NB_IoT(void);

void free_ul_ref_sigs_NB_IoT(void);
			 
#endif
