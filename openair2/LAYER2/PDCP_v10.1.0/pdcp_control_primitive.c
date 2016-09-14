/***************************************************************************
                        pdcp_control_primitives.c
                             -------------------
    begin                : Mon Dec 10 2001
    email                : Lionel.Gauthier@eurecom.fr
                             -------------------
    description
    This file contains the functions used for configuration of pdcp

 ***************************************************************************/
#include "rtos_header.h"
#include "platform.h"
#include "protocol_vars_extern.h"
#include "print.h"
//-----------------------------------------------------------------------------
#include "rlc.h"
#include "pdcp.h"
#include "debug_l2.h"
//-----------------------------------------------------------------------------
void
configure_pdcp_req (struct pdcp_entity *pdcpP, void *rlcP, uint8_t rlc_sap_typeP, uint8_t header_compression_typeP)
{
  //-----------------------------------------------------------------------------
  mem_block      *mb;

  mb = get_free_mem_block (sizeof (struct cpdcp_primitive));
  ((struct cpdcp_primitive *) mb->data)->type = CPDCP_CONFIG_REQ;
  ((struct cpdcp_primitive *) mb->data)->primitive.config_req.rlc_sap = rlcP;
  ((struct cpdcp_primitive *) mb->data)->primitive.config_req.rlc_type_sap = rlc_sap_typeP;
  ((struct cpdcp_primitive *) mb->data)->primitive.config_req.header_compression_type = header_compression_typeP;
  send_pdcp_control_primitive (pdcpP, mb);
}
