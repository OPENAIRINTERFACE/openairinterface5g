/***************************************************************************
                            pdcp_control_primitives_proto_extern.h
                             -------------------
    begin                : Mon Dec 10 2001
    email                : Lionel.Gauthier@eurecom.fr
                             -------------------
    description
    This file contains the prototypes of functions used for configuration of pdcp

 ***************************************************************************/
#ifndef PDCP_CONTROL_PRIMITIVES_PROTO_EXTERN_H
#    define PDCP_CONTROL_PRIMITIVES_PROTO_EXTERN_H
#    include "pdcp.h"
void            rrc_configure_pdcp (struct pdcp_entity *pdcpP, void *rlcP, uint8_t rlc_sap_typeP, uint8_t header_compression_typeP);
#endif
