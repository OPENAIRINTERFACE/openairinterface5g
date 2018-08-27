#include "platform_types.h"
#include "rrc_defs.h"
#include "rrc_extern.h"
#include "UTIL/LOG/log.h"
#include "pdcp.h"
#include "msc.h"
#include "common/ran_context.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

extern RAN_CONTEXT_t RC;

int8_t mac_rrc_nr_data_req(const module_id_t Mod_idP,
                           const int         CC_id,
                           const frame_t     frameP,
                           const rb_id_t     Srb_id,
                           const uint8_t     Nb_tb,
                           uint8_t *const    buffer_pP ){

  asn_enc_rval_t enc_rval;
  //SRB_INFO *Srb_info;
  //uint8_t Sdu_size                = 0;
  uint8_t sfn_msb                     = (uint8_t)((frameP>>4)&0x3f);
  
#ifdef DEBUG_RRC
  int i;
  LOG_I(RRC,"[eNB %d] mac_rrc_data_req to SRB ID=%d\n",Mod_idP,Srb_id);
#endif

  gNB_RRC_INST *rrc;
  rrc_gNB_carrier_data_t *carrier;
  NR_BCCH_BCH_Message_t *mib;
  
  rrc     = RC.nrrrc[Mod_idP];
  carrier = &rrc->carrier[0];
  mib     = &carrier->mib;

  if( (Srb_id & RAB_OFFSET ) == MIBCH) {
    mib->message.choice.mib->systemFrameNumber.buf[0] = sfn_msb << 2;
    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_BCCH_BCH_Message,
                                     NULL,
                                     (void *)mib,
                                     carrier->MIB,
                                     24);
    LOG_I(NR_RRC,"Encoded MIB for frame %d sfn_msb %d (%p), bits %lu\n",frameP,sfn_msb,carrier->MIB,enc_rval.encoded);
    buffer_pP[0]=carrier->MIB[0];
    buffer_pP[1]=carrier->MIB[1];
    buffer_pP[2]=carrier->MIB[2];
    LOG_I(NR_RRC,"MIB PDU buffer_pP[0]=%x , buffer_pP[1]=%x, buffer_pP[2]=%x\n",buffer_pP[0],buffer_pP[1],buffer_pP[2]);
    AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n",
                 enc_rval.failed_type->name, enc_rval.encoded);
    return(3);
  }

//BCCH SIB1 SIBs

//CCCH

  return(0);

}
