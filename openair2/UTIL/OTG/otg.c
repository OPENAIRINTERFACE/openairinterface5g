/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file otg.c
* \brief common function for otc tx and rx
* \author N. Nikaein and A. Hafsaoui
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/



#include "otg.h"
#include "otg_vars.h"


// Defining initial and default values of OTG structures

void init_all_otg(int max_nb_frames)
{

  //set otg params to 0
  g_otg = calloc(1, sizeof(otg_t));

  if (g_otg == NULL)
    /* Memory could not be allocated */
    LOG_E(OTG,"Couldn't allocate memory for otg_t\n");

  memset(g_otg, 0, sizeof(otg_t));

  g_otg_multicast = calloc(1, sizeof(otg_multicast_t));

  if (g_otg_multicast == NULL)
    /* Memory could not be allocated */
    LOG_E(OTG,"Couldn't allocate memory for otg_multicast_t\n");

  memset(g_otg_multicast, 0, sizeof(otg_multicast_t));

  //set otg infos to 0
  otg_info = calloc(1, sizeof(otg_info_t));

  if (otg_info == NULL)
    /* Memory could not be allocated */
    LOG_E(OTG,"Couldn't allocate memory for otg_info_t\n");

  memset(otg_info, 0, sizeof(otg_info_t));

  //set otg multicast infos to 0
  otg_multicast_info = calloc(1, sizeof(otg_multicast_info_t));

  if (otg_multicast_info == NULL)
    /* Memory could not be allocated */
    LOG_E(OTG,"Couldn't allocate memory for otg_multicast_info_t\n");

  memset(otg_multicast_info, 0, sizeof(otg_multicast_info_t));

  //set otg forms infos to 0
  otg_forms_info=calloc(1, sizeof(otg_forms_info_t));

  if (otg_forms_info == NULL)
    /* Memory could not be allocated */
    LOG_E(OTG,"Couldn't allocate memory for otg_forms_info_t\n");

  memset(otg_forms_info, 0, sizeof(otg_forms_info_t));

  g_otg->max_nb_frames=max_nb_frames;

  LOG_I(OTG,"init done: init_all_otg\n");

}

char *str_sub (const char *s, unsigned int start, unsigned int end)
{

  char *new_s = NULL;
  int i;

  if (s != NULL && start < end)   {
    new_s = malloc (sizeof (*new_s) * (end - start + 2));

    if (new_s != NULL) {
      for (i = start; i <= end; i++) {
        new_s[i-start] = s[i];
      }

      new_s[i-start] = '\0';
    } else {
      LOG_E(OTG,"Insufficient memory \n");
      exit (-1);
    }
  }

  return new_s;
}

// set the simulation time
void set_ctime(int ctime)
{
  otg_info->ctime=ctime;
  //  otg_muticast_info->ctime=ctime;

}



// get the simulation time
int get_ctime(void)
{
  return otg_info->ctime;
}


void free_otg()
{
  int i,j,k;

  for (i=0; i<g_otg->num_nodes; i++)
    for (j=0; j<g_otg->num_nodes; j++)
      free( g_otg->dst_ip[i][j] );

  free( g_otg );
  g_otg = 0;

  LOG_D(OTG,"DEBUG TARMA: free_otg() called \n");

  for(i=0; i<NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
    for(j=0; j<NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; j++) {
      for(k=0; k<MAX_NUM_APPLICATION; k++) {
        if(otg_info->tarma_stream[i][j][k]) {
          free(otg_info->tarma_stream[i][j][k]);
          LOG_D(OTG,"DEBUG TARMA: freed tarma_stream[%d][%d][%d]\n",i,j,k);
        }

        if(otg_info->tarma_video[i][j][k]) {
          free(otg_info->tarma_video[i][j][k]);
          LOG_D(OTG,"DEBUG TARMA: freed tarma_video[%d][%d][%d]\n",i,j,k);
        }

        if(otg_info->background_stream[i][j][k]) {
          free(otg_info->background_stream[i][j][k]);
          LOG_D(OTG,"DEBUG TARMA: freed background_stream[%d][%d][%d]\n",i,j,k);
        }
      }
    }
  }

  free(otg_info);
}

