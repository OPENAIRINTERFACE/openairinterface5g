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

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

#ifndef __INTERFACE_H__
#    define __INTERFACE_H__


typedef struct {
  void*           pointer;
} memory_reference_t2;



typedef struct {
  memory_reference_t2  mem_ref;
} obj_ref_t2;

typedef struct {
  obj_ref_t2 *gm;
} HO;


typedef struct {
  void*           pointer;
  int port;
  int Exec_Msg_Flag;
  int node_id;
  int node_role ;
  int next_slot;
  int last_slot;
  int frame;
  int sub_frame;
  int slot;
  int Exec_FLAG;
  int EResp_FLAG;
  int ue_id;
} s_t;

HO Instance[3];


typedef struct {
  int thread_id;
  int eNB_id;
  int UE_id;
  double **s_re;
  double **s_im;
  double **r_re;
  double **r_im;
  double **r_re0;
  double **r_im0;
  struct channel_desc_t *eNB2UE;
  struct channel_desc_t *UE2eNB;
  struct node_desc_t *enb_data;
  struct node_desc_t *ue_data;
  int *next_slot;
  int *abstraction_flag;
  struct LTE_DL_FRAME_PARMS *frame_parms;
  int **tx_data[3];
  int **rx_data[3];
} ch_thread;


#define CHANNEL_PORT 37800
#define eNB_PORT 38800
#define UE_PORT 39800
#define MAX_eNB 20
#define MAX_UE 20
#endif
