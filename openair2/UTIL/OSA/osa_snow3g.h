/*******************************************************************************

  Eurecom OpenAirInterface
  Copyright(c) 1999 - 2014 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fr/openairinterface
  Address      : EURECOM, Campus SophiaTech, 450 Route des Chappes
                 06410 Biot FRANCE


*******************************************************************************/
/*! \file snow3g.h
* \brief
* \author Open source Adapted from Specification of the 3GPP Confidentiality and
*          Integrity Algorithms UEA2 & UIA2. Document 2: SNOW 3G Specification
* \integrators  Kharbach Othmane, GAUTHIER Lionel.
* \date 2014
* \version
* \note
* \bug
* \warning
*/
#ifndef OSA_SNOW3G_H_
#define OSA_SNOW3G_H_

typedef struct osa_snow_3g_context_s {
  uint32_t LFSR_S0;
  uint32_t LFSR_S1;
  uint32_t LFSR_S2;
  uint32_t LFSR_S3;
  uint32_t LFSR_S4;
  uint32_t LFSR_S5;
  uint32_t LFSR_S6;
  uint32_t LFSR_S7;
  uint32_t LFSR_S8;
  uint32_t LFSR_S9;
  uint32_t LFSR_S10;
  uint32_t LFSR_S11;
  uint32_t LFSR_S12;
  uint32_t LFSR_S13;
  uint32_t LFSR_S14;
  uint32_t LFSR_S15;

  /* FSM : The Finite State Machine has three 32-bit registers R1, R2 and R3.
  */
  uint32_t FSM_R1;
  uint32_t FSM_R2;
  uint32_t FSM_R3;
} osa_snow_3g_context_t;

/* Initialization.
* Input k[4]: Four 32-bit words making up 128-bit key.
* Input IV[4]: Four 32-bit words making 128-bit initialization variable.
* Output: All the LFSRs and FSM are initialized for key generation.
*/
void osa_snow3g_initialize(uint32_t k[4], uint32_t IV[4], osa_snow_3g_context_t *snow_3g_context_pP);

/* Generation of Keystream.
* input n: number of 32-bit words of keystream.
* input z: space for the generated keystream, assumes
* memory is allocated already.
* output: generated keystream which is filled in z
*/

void osa_snow3g_generate_key_stream(uint32_t n, uint32_t *z, osa_snow_3g_context_t *snow_3g_context_pP);

#endif
