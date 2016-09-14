/*
 * nas_messages_def.h
 *
 *  Created on: Jan 07, 2014
 *      Author: winckel
 */

#if defined(ENABLE_USE_MME) && defined(ENABLE_NAS_UE_LOGGING)
//-------------------------------------------------------------------------------------------//
// Messages for NAS logging
MESSAGE_DEF(NAS_DL_EMM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_dl_emm_raw_msg)
MESSAGE_DEF(NAS_UL_EMM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_ul_emm_raw_msg)

MESSAGE_DEF(NAS_DL_EMM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_emm_plain_msg_t,        nas_dl_emm_plain_msg)
MESSAGE_DEF(NAS_UL_EMM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_emm_plain_msg_t,        nas_ul_emm_plain_msg)
MESSAGE_DEF(NAS_DL_EMM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_emm_protected_msg_t,    nas_dl_emm_protected_msg)
MESSAGE_DEF(NAS_UL_EMM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_emm_protected_msg_t,    nas_ul_emm_protected_msg)

MESSAGE_DEF(NAS_DL_ESM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_dl_esm_raw_msg)
MESSAGE_DEF(NAS_UL_ESM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_ul_esm_raw_msg)

MESSAGE_DEF(NAS_DL_ESM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_esm_plain_msg_t,        nas_dl_esm_plain_msg)
MESSAGE_DEF(NAS_UL_ESM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_esm_plain_msg_t,        nas_ul_esm_plain_msg)
MESSAGE_DEF(NAS_DL_ESM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_esm_protected_msg_t,    nas_dl_esm_protected_msg)
MESSAGE_DEF(NAS_UL_ESM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_esm_protected_msg_t,    nas_ul_esm_protected_msg)

//-------------------------------------------------------------------------------------------//
#endif /* ENABLE_USE_MME */
