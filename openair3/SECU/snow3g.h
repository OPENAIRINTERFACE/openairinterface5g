/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

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
#ifndef SNOW3G_H_
#define SNOW3G_H_

typedef struct snow_3g_context_s {
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
} snow_3g_context_t;

/* Initialization.
* Input k[4]: Four 32-bit words making up 128-bit key.
* Input IV[4]: Four 32-bit words making 128-bit initialization variable.
* Output: All the LFSRs and FSM are initialized for key generation.
*/
void snow3g_initialize(uint32_t k[4], uint32_t IV[4], snow_3g_context_t *snow_3g_context_pP);

/* Generation of Keystream.
* input n: number of 32-bit words of keystream.
* input z: space for the generated keystream, assumes
* memory is allocated already.
* output: generated keystream which is filled in z
*/

void snow3g_generate_key_stream(uint32_t n, uint32_t *z, snow_3g_context_t *snow_3g_context_pP);

#endif
