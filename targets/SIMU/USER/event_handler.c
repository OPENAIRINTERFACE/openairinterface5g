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

/*! \file event_handler.c
* \brief event handler primitives
* \author Navid Nikaein and Mohamed Said MOSLI BOUKSIAA,
* \date 2014
* \version 0.5
* @ingroup _oai
*/

#include <stdio.h>
#include <stdlib.h>
// #include "UTIL/LOG/log.h"
#include "event_handler.h"

//extern frame_t frame;
//End_Of_Sim_Event end_event; //Could later be a list of condition_events (if the end condition is more complicated)

Event_List_t event_list;

void add_event(Event_t event)
{

  Event_elt_t * counter = event_list.head;
  Event_elt_t * previous = counter;
  Event_elt_t * elt = malloc(sizeof(Event_elt_t));

  while (counter != NULL) {
    if ((counter->event).frame > event.frame) break;
    //else if ((counter->event).next_slot > event.next_slot) break;
    else {
      previous = counter;
      counter = counter->next;
    }
  }

  elt->event = event;

  //printf("%" PRIu8 "testestest \n", *((uint8_t *) event.value));
  if (event_list.head != NULL)
    event_list_add_element(elt, previous, &event_list);
  else
    event_list_add_head(elt, &event_list);

}

void schedule(Operation_Type_t op, Event_Type_t type, int frame, char * key, void * value, int ue, int lcid)
{
  Event_t event;
  event.frame = frame;

  if (key == NULL) { //Global model update
    event.key = NULL;
  } else { //Update of a single parameter
    event.key = malloc (sizeof(char) * (strlen(key)+1));
    strcpy(event.key, key);
  }

  event.value = value;
  //printf("%" PRIu8 "testestest \n",*((uint8_t *)value));
  event.type = type;
  event.optype = op;
  event.lcid = lcid;
  event.ue = ue;
  add_event(event);
}

void schedule_delayed(Operation_Type_t op, Event_Type_t type, char * key, void* value, char * date, int ue, int lcid)
{
  printf("Delayed schedule");
  printf("date %s\n", date);
  // first get the time from the system
  struct tm *pdh;
  time_t intps;
  intps=time(NULL);
  pdh = localtime(&intps);
  char *date_cpy = malloc (sizeof (char) * 256);
  strncpy( date_cpy, date, 256 );
  date_cpy[255] = 0; // terminate string
  // second count the frame to reach the time
  char *heure = NULL;
  int heure_int;
  int minute_int;
  int heure_diff;
  int minute_diff;
  int ms_diff;
  int frame;


  char *minute = NULL;
  heure = strtok(date_cpy,"h");
  //heure = strchr(date,'h');
  printf("hour char : %s", heure);

  minute = strchr(date,'h');
  printf("minute char : %s", minute+1);

  free( date_cpy );
  date_cpy = 0;

  heure_int = atoi(heure);

  minute_int = atoi(minute+1);
  heure_diff = heure_int - pdh->tm_hour;
  minute_diff = minute_int - pdh->tm_min;
  ms_diff = heure_diff*3600*1000 + minute_diff*60*1000;
  frame = ms_diff/10;

  /*char stTemp[2];
  int valeur_a_en_entier;

  sprintf(stTemp,"%c",a);
  valeur_a_en_entier = atoi(stTemp);*/
  // third schedule with the classic function
  printf("schedule_frame %d\n",frame);
  printf("test pdh hr : %d\n", pdh->tm_hour);
  printf("test heure_diff : %d\n", heure_diff);
  printf("test minute_diff : %d\n", minute_diff);

  if(frame >= 0) {
    schedule(op,type,frame,key,value,lcid,ue);
  }
}

/*
 * this function can reschedule events in the future to build a desired (more dynamic) scenario
 * TODO: make sure that OCG OAI_Emulation is decoupled with the local vars
 *
 * For ET_MAC parameters are first set in the OAI_emulation (OCG) structure and then
 * duplicated in the eNB_mac_inst structure
 *
 * If you want to modifiy a parameter for all UEs, put -1 in the lcid and the ue field when you call
 * the schedule function
 */
void schedule_events()
{

  /*
   * Step 1: vars needed for future changes
   */

  // overall config associated with OAI_ET
  // OAI_Emulation * oai_frame_1000;

  // specific config
  /*
   * associated with SYS_ET
   */
  //Environment_System_Config * system_frame_200;
  /*
   * associated with TOPO_ET
   */
  //Topology_Config *topology_frame_10;
  /*
   * associated with APP_ET
   */
  //Application_Config *application_frame_20;
  /*
   * associated with EMU_ET
   */
  //Emulation_Config * emulation_frame_100;
  uint8_t a;

  Mac_config* mac_config;
  Mac_config* mac_config2;
  Mac_config* mac_config3;
  Mac_config* mac_config4;
  Mac_config* mac_config5;
  int i=0;

  mac_config = NULL;
  mac_config2 = NULL;
  mac_config3 = NULL;
  mac_config4 = NULL;
  mac_config5 = NULL;
  mac_config = malloc(sizeof(Mac_config));
  mac_config2= malloc(sizeof(Mac_config));
  mac_config3= malloc(sizeof(Mac_config));
  mac_config4= malloc(sizeof(Mac_config));
  mac_config5= malloc(sizeof(Mac_config));

  a = 5;

  //uint16_t *priority[11];//={1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2};


  //int tab[NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX];
  Application_Config* application_config;
  application_config = malloc(sizeof(Application_Config));

  for(i=0; i<NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
    //tab[i]=i;
    application_config->customized_traffic.aggregation_level[i]=i;
  }

  mac_config->DCI_aggregation_min = a;
  mac_config->DLSCH_dci_size_bits = a;

  /*
   * Step 2: set the desired future changes in the vars
   */
  // i.e. optionally provide an XML file and update OCG vars

  //mob_frame_10 -> ...
  //application_frame_30 -> ...

  /*
   * Step 3: schedule the execution of changes
   */

  // general OAI dynamic configuration

  //schedule(OAI_ET, 1000, NULL, oai_frame_1000);
  //schedule(SYS_ET, 200, NULL, system_frame_200);
  //schedule(READ, TOPO_ET, 10, NULL, "");
  //schedule(APP_ET, 20, NULL, application_frame_20);
  //schedule(EMU_ET, 100, NULL,emulation_frame_100);

  // protocol dynamic configuration
  schedule(READ, MAC_ET, 115, NULL, "",0,0);
  //schedule(READ, MAC_ET, 400, NULL, "");
  //schedule(READ, MAC_ET, 500, NULL, "");
  //schedule(WRITE, MAC_ET, 100, NULL, "");
  //schedule(READ, TOPO_ET,110,NULL,"",0,0);
  //double b = (double) 45;
  //schedule(READ, APP_ET,111,NULL,"",0,0);
  //schedule(READ, SYS_ET,112,NULL,"",0,0);
  //schedule(READ, EMU_ET,113,NULL,"",-1,-1);

  //schedule(WRITE, MAC_ET, 122, "DCI_aggregation_min", mac_config);
  //schedule(WRITE, MAC_ET, 123, "max_allowed_rbs", mac_config);
  //schedule(WRITE, MAC_ET, 125, "max_mcs", mac_config);
  int j=0;

  for(i=0; i<NUMBER_OF_UE_MAX; i++) {
    mac_config2[i].DCI_aggregation_min = 1;

    for(j=0; j<MAX_NUM_LCID; j++) {
      mac_config2[i].ul_bandwidth[j]= (uint8_t) 4;
      mac_config2[i].max_mcs[j]= 1;
    }
  }

  //schedule(READ or WRITE, ET_MAC, frame number, key, mac_config, ue, lcid);
  //schedule(WRITE, MAC_ET, 125, "ul_bandwidth", mac_config2,-1,-1);
  //schedule(WRITE, MAC_ET, 126, "DCI_aggregation_min", mac_config2,-1,-1);
  //schedule(WRITE, MAC_ET, 128, "max_mcs", mac_config2,-1,-1);
  mac_config[0].max_mcs[3]= 4;
  schedule(WRITE, MAC_ET, 140, "max_mcs", mac_config,0,3);
  mac_config2[0].max_mcs[3]= 26;
  schedule(WRITE, MAC_ET, 1500, "max_mcs", mac_config2,0,3);
  mac_config3[0].max_mcs[3]= 3;
  schedule(WRITE, MAC_ET, 1600, "max_mcs", mac_config3,0,3);
  mac_config4[0].max_mcs[3]= 26;
  schedule(WRITE, MAC_ET, 2400, "max_mcs", mac_config4,0,3);
  mac_config5[0].max_mcs[3]= 13;
  schedule(WRITE, MAC_ET, 3000, "max_mcs", mac_config5,0,3);


  printf("schedule_event\n");

  event_list_display(&event_list);

}

void execute_events(frame_t frame)
{

  Event_elt_t *user_defined_event;
  Event_t event;

  while ((user_defined_event = event_list_get_head(&event_list)) != NULL) {

    event = user_defined_event->event;

    if (event.frame == frame) {
      //printf("%" PRIu8 "testestest \n", *((uint8_t *) event.value));
      switch (event.type) {
      case OAI_ET:
        update_oai_model(event.key, event.value);
        user_defined_event = event_list_remove_head(&event_list);
        break;

      case SYS_ET:
        update_sys_model(event);
        user_defined_event = event_list_remove_head(&event_list);
        break;

      case TOPO_ET:
        update_topo_model(event); //implement it with assigning the new values to that of oai_emulation & second thing is to ensure mob model is always read from oai_emulation
        user_defined_event = event_list_remove_head(&event_list);
        break;

      case APP_ET:
        update_app_model(event);
        user_defined_event = event_list_remove_head(&event_list);
        break;

      case EMU_ET:
        update_emu_model(event);
        user_defined_event = event_list_remove_head(&event_list);
        break;

      case MAC_ET:
        update_mac(event);
        user_defined_event = event_list_remove_head(&event_list);
        break;

      default :
        break;
      }
    } else {
      break;
    }
  }

}


void update_mac(Event_t event)
{
  LOG_I(EMU,"A NEW MAC MODEL\n");
  int i = 0;
  UE_list_t *UE_list = 0;
  int enb_module_id=0;
  int cc_id =0;
  eNB_MAC_INST *eNB_mac_inst = get_eNB_mac_inst(enb_module_id);
  OAI_Emulation *oai_emulation  = get_OAI_emulation();

  if(event.optype == READ) {

    printf("eNB_stats\n\n\n");

    if(event.key==NULL) {

      LOG_I(EMU,"num_dlactive_UEs :");
      printf("%" PRIu16 "\n",eNB_mac_inst->eNB_stats[cc_id].num_dlactive_UEs);
      LOG_I(EMU,"available_prbs :");
      printf("%" PRIu16 "\n",eNB_mac_inst->eNB_stats[cc_id].available_prbs);
      LOG_I(EMU,"total_available_prbs :");
      printf("%" PRIu32 "\n", eNB_mac_inst->eNB_stats[cc_id].total_available_prbs);
      LOG_I(EMU,"available_ncces :");
      printf("%" PRIu16 "\n",eNB_mac_inst->eNB_stats[cc_id].available_ncces);
      LOG_I(EMU,"dlsch_bitrate :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].dlsch_bitrate);
      LOG_I(EMU,"dlsch_bytes_tx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].dlsch_bytes_tx);
      LOG_I(EMU,"dlsch_pdus_tx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].dlsch_pdus_tx);
      LOG_I(EMU,"total_dlsch_bitrate :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].total_dlsch_bitrate);
      LOG_I(EMU,"total_dlsch_bytes_tx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].total_dlsch_bytes_tx);
      LOG_I(EMU,"total_dlsch_pdus_tx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].total_dlsch_pdus_tx);
      LOG_I(EMU,"ulsch_bitrate :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].ulsch_bitrate);
      LOG_I(EMU,"ulsch_bytes_rx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].ulsch_bytes_rx);
      LOG_I(EMU,"ulsch_pdus_rx :");
      printf("%" PRIu64 "\n",eNB_mac_inst->eNB_stats[cc_id].ulsch_pdus_rx);
    } else if(!strcmp((char *) event.key, "num_dlactive_UEs")) {
      LOG_I(EMU,"num_dlactive_UEs :");
      printf("%" PRIu16 "\n",eNB_mac_inst->eNB_stats[cc_id].num_dlactive_UEs);
    } else if(!strcmp((char *) event.key, "available_prbs")) {
      LOG_I(EMU,"available_prbs :");
      printf("%" PRIu16 "\n",eNB_mac_inst->eNB_stats[cc_id].available_prbs);
    } else if(!strcmp((char *) event.key, "total_available_prbs")) {
      LOG_I(EMU,"total_available_prbs :");
      printf("%" PRIu32 "\n", eNB_mac_inst->eNB_stats[cc_id].total_available_prbs);
    } else if(!strcmp((char *) event.key, "available_ncces")) {
      LOG_I(EMU,"available_ncces :");
      printf("%" PRIu16 "\n",eNB_mac_inst->eNB_stats[cc_id].available_ncces);
    } else if(!strcmp((char *) event.key, "dlsch_bitrate")) {
      LOG_I(EMU,"dlsch_bitrate :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].dlsch_bitrate);
    } else if(!strcmp((char *) event.key, "dlsch_bytes_tx")) {
      LOG_I(EMU,"dlsch_bytes_tx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].dlsch_bytes_tx);
    } else if(!strcmp((char *) event.key, "dlsch_pdus_tx")) {
      LOG_I(EMU,"dlsch_pdus_tx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].dlsch_pdus_tx);
    } else if(!strcmp((char *) event.key, "total_dlsch_bitrate")) {
      LOG_I(EMU,"total_dlsch_bitrate :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].total_dlsch_bitrate);
    } else if(!strcmp((char *) event.key, "total_dlsch_bytes_tx")) {
      LOG_I(EMU,"total_dlsch_bytes_tx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].total_dlsch_bytes_tx);
    } else if(!strcmp((char *) event.key, "total_dlsch_pdus_tx")) {
      LOG_I(EMU,"total_dlsch_pdus_tx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].total_dlsch_pdus_tx);
    } else if(!strcmp((char *) event.key, "ulsch_bitrate")) {
      LOG_I(EMU,"ulsch_bitrate :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].ulsch_bitrate);
    } else if(!strcmp((char *) event.key, "ulsch_bytes_rx")) {
      LOG_I(EMU,"ulsch_bytes_rx :");
      printf("%" PRIu32 "\n",eNB_mac_inst->eNB_stats[cc_id].ulsch_bytes_rx);
    } else if(!strcmp((char *) event.key, "ulsch_pdus_rx")) {
      LOG_I(EMU,"ulsch_pdus_rx :");
      printf("%" PRIu64 "\n",eNB_mac_inst->eNB_stats[cc_id].ulsch_pdus_rx);
    }
  } else if(event.optype == WRITE) {
    LOG_I(EMU,"WRITE OPERATION \n");


    if(event.key==NULL && event.value!=NULL) {

      if((Mac_config *) event.value !=NULL && validate_mac(event)) {

        Mac_config* mac_config;
        mac_config = (Mac_config *) event.value;
        LOG_I(EMU,"update complete mac params \n");

        if(event.ue == -1 && event.lcid == -1) {
          UE_list = &eNB_mac_inst->UE_list;

          for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
	    /*
            if(&mac_config[i].DCI_aggregation_min) {
              LOG_I(EMU,"update dci aggregation min\n");
              oai_emulation->mac_config[i].DCI_aggregation_min= mac_config[i].DCI_aggregation_min;

              UE_list->UE_template[0][i].DCI_aggregation_min = oai_emulation->mac_config[i].DCI_aggregation_min;
              LOG_I(EMU,"DCI_aggregation_min UE %d: \n",i);
              LOG_I(EMU,"%" PRIu8,UE_list->UE_template[0][i].DCI_aggregation_min);
            }

            if(&mac_config[i].DLSCH_dci_size_bits !=NULL) {
              oai_emulation->mac_config[i].DLSCH_dci_size_bits= mac_config[i].DLSCH_dci_size_bits;
              UE_list->UE_template[0][i].DLSCH_dci_size_bits = oai_emulation->mac_config[i].DLSCH_dci_size_bits;
              LOG_I(EMU,"DLSCH_dci_size_bits UE %d: \n",i);
              LOG_I(EMU,"%" PRIu8,UE_list->UE_template[0][i].DLSCH_dci_size_bits);
            }
	    */

            if(mac_config[i].priority !=NULL) {
              LOG_I(EMU,"update priority \n");
              int j;
              for(j=0; j<MAX_NUM_LCID; j++) {

                if(&mac_config[i].priority[j]!=NULL) {
                  oai_emulation->mac_config[i].priority[j]= mac_config[i].priority[j];
                  UE_list->UE_sched_ctrl[i].priority[j] = oai_emulation->mac_config[i].priority[j];
                  LOG_I(EMU,"priority UE %d LCID %d:",i,j);
                  LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].priority[j]);
                }
              }
            }

            if(&mac_config[i].ul_bandwidth !=NULL) {

              int j=0;

              for(j=0; j<MAX_NUM_LCID; j++) {

                oai_emulation->mac_config[i].ul_bandwidth[j]= mac_config[i].ul_bandwidth[j];
                UE_list->UE_sched_ctrl[i].ul_bandwidth[j] = oai_emulation->mac_config[i].ul_bandwidth[j];
                LOG_I(EMU,"ul_bandwidth UE %d LCID %d:", i, j);
                LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].ul_bandwidth[j]);
              }
            }

            if(&mac_config[i].dl_bandwidth !=NULL) {
              int j=0;

              for(j=0; j<MAX_NUM_LCID; j++) {

                oai_emulation->mac_config[i].dl_bandwidth[j]= mac_config[i].dl_bandwidth[j];
                UE_list->UE_sched_ctrl[i].dl_bandwidth[j] = oai_emulation->mac_config[i].dl_bandwidth[j];
                LOG_I(EMU,"ul_bandwidth UE %d LCID %d:", i, j);
                LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].dl_bandwidth[j]);
              }
            }

            if(&mac_config[i].ue_AggregatedMaximumBitrateDL !=NULL) {
              oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateDL= mac_config[i].ue_AggregatedMaximumBitrateDL;
              UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateDL = oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateDL;
              LOG_I(EMU,"ue_AggregatedMaximumBitrateDL UE %d:",i );
              LOG_I(EMU,"%" PRIu64 "\n",UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateDL);
            }

            if(&mac_config[i].ue_AggregatedMaximumBitrateUL !=NULL) {
              oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateUL= mac_config[i].ue_AggregatedMaximumBitrateUL;
              UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateUL = oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateUL;
              LOG_I(EMU,"ue_AggregatedMaximumBitrateUL UE %d:",i);
              LOG_I(EMU,"%" PRIu64 "\n",UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateUL);
            }

            if(&mac_config[i].cqiSchedInterval !=NULL) {
              oai_emulation->mac_config[i].cqiSchedInterval= mac_config[i].cqiSchedInterval;
              UE_list->UE_sched_ctrl[i].cqiSchedInterval = oai_emulation->mac_config[i].cqiSchedInterval;
              LOG_I(EMU,"cqiSchedInterval UE %d:",i);
              LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[i].cqiSchedInterval);
            }

            if(&mac_config[i].mac_ContentionResolutionTimer !=NULL) {
              oai_emulation->mac_config[i].mac_ContentionResolutionTimer= mac_config[i].mac_ContentionResolutionTimer;
              UE_list->UE_sched_ctrl[i].mac_ContentionResolutionTimer = oai_emulation->mac_config[i].mac_ContentionResolutionTimer;
              LOG_I(EMU,"mac_ContentionResolutionTimer UE %d:", i);
              LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].mac_ContentionResolutionTimer);
            }

            if(mac_config->max_allowed_rbs !=NULL) {
              int j=0;

              for(j=0; j<MAX_NUM_LCID; j++) {
                oai_emulation->mac_config[i].max_allowed_rbs[j]= mac_config[i].max_allowed_rbs[j];
                UE_list->UE_sched_ctrl[i].max_rbs_allowed_slice[j][0] = oai_emulation->mac_config[i].max_allowed_rbs[j];
                LOG_I(EMU,"max_allowed_rbs UE %d LCID %d:",i,j);
                LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[i].max_rbs_allowed_slice[j][0]);
              }
            }

            if(mac_config[i].max_mcs !=NULL) {
              int j=0;

              for(j=0; j<MAX_NUM_LCID; j++) {
                oai_emulation->mac_config[i].max_mcs[j]= mac_config[i].max_mcs[j];
                UE_list->UE_sched_ctrl[i].max_mcs[j] = oai_emulation->mac_config[i].max_mcs[j];
                LOG_I(EMU,"max_mcs UE %d LCID %d:",i,j);
                LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].max_mcs[j]);
              }

            }
          }
        } else {
          Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
          mac_config = (Mac_config *) event.value;
          LOG_I(EMU,"update complete mac params \n");
          i = event.ue;
          int j = event.lcid;
	  /*
          if(&mac_config[i].DCI_aggregation_min) {
            LOG_I(EMU,"update dci aggregation min\n");
            oai_emulation->mac_config[i].DCI_aggregation_min= mac_config[i].DCI_aggregation_min;
            // duplication dans le ue template, vérifier la validité avant
            UE_list->UE_template[0][i].DCI_aggregation_min = oai_emulation->mac_config[i].DCI_aggregation_min;
            LOG_I(EMU,"DCI_aggregation_min UE %d: \n",i);
            LOG_I(EMU,"%" PRIu8,UE_list->UE_template[0][i].DCI_aggregation_min);
          }

          if(&mac_config[i].DLSCH_dci_size_bits !=NULL) {
            oai_emulation->mac_config[i].DLSCH_dci_size_bits= mac_config[i].DLSCH_dci_size_bits;
            UE_list->UE_template[0][i].DLSCH_dci_size_bits = oai_emulation->mac_config[i].DLSCH_dci_size_bits;
            LOG_I(EMU,"DLSCH_dci_size_bits UE %d: \n",i);
            LOG_I(EMU,"%" PRIu8,UE_list->UE_template[0][i].DLSCH_dci_size_bits);
          }
	  */
          if(mac_config[i].priority !=NULL) {
            LOG_I(EMU,"update priority \n");


            if(&mac_config[i].priority[j]!=NULL) {
              oai_emulation->mac_config[i].priority[j]= mac_config[i].priority[j];
              UE_list->UE_sched_ctrl[i].priority[j] = oai_emulation->mac_config[i].priority[j];
              LOG_I(EMU,"priority UE %d LCID %d:",i,j);
              LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].priority[j]);
            }

          }

          if(&mac_config[i].ul_bandwidth !=NULL) {
            // faire boucle par ue puis par lcid



            oai_emulation->mac_config[i].ul_bandwidth[j]= mac_config[i].ul_bandwidth[j];
            UE_list->UE_sched_ctrl[i].ul_bandwidth[j] = oai_emulation->mac_config[i].ul_bandwidth[j];
            LOG_I(EMU,"ul_bandwidth UE %d LCID %d:", i, j);
            LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].ul_bandwidth[j]);

          }

          if(&mac_config[i].dl_bandwidth !=NULL) {
            oai_emulation->mac_config[i].dl_bandwidth[j]= mac_config[i].dl_bandwidth[j];
            UE_list->UE_sched_ctrl[i].dl_bandwidth[j] = oai_emulation->mac_config[i].dl_bandwidth[j];
            LOG_I(EMU,"ul_bandwidth UE %d LCID %d:", i, j);
            LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].dl_bandwidth[j]);

          }

          if(&mac_config[i].min_ul_bandwidth !=NULL) {
            // faire boucle par ue puis par lcid
            oai_emulation->mac_config[i].min_ul_bandwidth[j]= mac_config[i].min_ul_bandwidth[j];
            UE_list->UE_sched_ctrl[i].min_ul_bandwidth[j] = oai_emulation->mac_config[i].min_ul_bandwidth[j];
            LOG_I(EMU,"min_ul_bandwidth UE %d LCID %d:", i, j);
            LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].min_ul_bandwidth[j]);

          }

          if(&mac_config[i].min_dl_bandwidth !=NULL) {
            oai_emulation->mac_config[i].min_dl_bandwidth[j]= mac_config[i].min_dl_bandwidth[j];
            UE_list->UE_sched_ctrl[i].min_dl_bandwidth[j] = oai_emulation->mac_config[i].min_dl_bandwidth[j];
            LOG_I(EMU,"min_dl_bandwidth UE %d LCID %d:", i, j);
            LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].min_dl_bandwidth[j]);

          }

          if(&mac_config[i].ue_AggregatedMaximumBitrateDL !=NULL) {
            oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateDL= mac_config[i].ue_AggregatedMaximumBitrateDL;
            UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateDL = oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateDL;
            LOG_I(EMU,"ue_AggregatedMaximumBitrateDL UE %d:",i );
            LOG_I(EMU,"%" PRIu64 "\n",UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateDL);
          }

          if(&mac_config[i].ue_AggregatedMaximumBitrateUL !=NULL) {
            oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateUL= mac_config[i].ue_AggregatedMaximumBitrateUL;
            UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateUL = oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateUL;
            LOG_I(EMU,"ue_AggregatedMaximumBitrateUL UE %d:",i);
            LOG_I(EMU,"%" PRIu64 "\n",UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateUL);
          }

          if(&mac_config[i].cqiSchedInterval !=NULL) {
            oai_emulation->mac_config[i].cqiSchedInterval= mac_config[i].cqiSchedInterval;
            UE_list->UE_sched_ctrl[i].cqiSchedInterval = oai_emulation->mac_config[i].cqiSchedInterval;
            LOG_I(EMU,"cqiSchedInterval UE %d:",i);
            LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[i].cqiSchedInterval);
          }

          if(&mac_config[i].mac_ContentionResolutionTimer !=NULL) {
            oai_emulation->mac_config[i].mac_ContentionResolutionTimer= mac_config[i].mac_ContentionResolutionTimer;
            UE_list->UE_sched_ctrl[i].mac_ContentionResolutionTimer = oai_emulation->mac_config[i].mac_ContentionResolutionTimer;
            LOG_I(EMU,"mac_ContentionResolutionTimer UE %d:", i);
            LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].mac_ContentionResolutionTimer);
          }

          if(mac_config->max_allowed_rbs !=NULL) {

            oai_emulation->mac_config[i].max_allowed_rbs[j]= mac_config[i].max_allowed_rbs[j];
            UE_list->UE_sched_ctrl[i].max_rbs_allowed_slice[j][0] = oai_emulation->mac_config[i].max_allowed_rbs[j];
            LOG_I(EMU,"max_allowed_rbs UE %d LCID %d:",i,j);
            LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[i].max_rbs_allowed_slice[j][0]);

          }

          if(mac_config[i].max_mcs !=NULL) {
            oai_emulation->mac_config[i].max_mcs[j]= mac_config[i].max_mcs[j];
            UE_list->UE_sched_ctrl[i].max_mcs[j] = oai_emulation->mac_config[i].max_mcs[j];
            LOG_I(EMU,"max_mcs UE %d LCID %d:",i,j);
            LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].max_mcs[j]);

          }
        }
      }
    } else if(!strcmp((char *) event.key, "priority") && event.value!=NULL && validate_mac(event)) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;
      LOG_I(EMU,"priority update \n");
      if(event.ue == -1) {
        for(i=0; i<MAX_NUM_LCID; i++) {
          for(j=0; j<MAX_NUM_LCID; j++) {

            if(&mac_config[i].priority[j]!=NULL) {
              oai_emulation->mac_config[i].priority[j]= mac_config[i].priority[j];
              UE_list->UE_sched_ctrl[i].priority[j] = oai_emulation->mac_config[i].priority[j];
              LOG_I(EMU,"priority UE %d LCID %d:",i,j);
              LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].priority[j]);
            }
          }
        }
      } else {
        oai_emulation->mac_config[event.ue].priority[event.lcid]= mac_config[event.ue].priority[event.lcid];
        UE_list->UE_sched_ctrl[event.ue].priority[event.lcid] = oai_emulation->mac_config[event.ue].priority[event.lcid];
        LOG_I(EMU,"priority UE %d LCID %d:",i,j);
        LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[event.ue].priority[event.lcid]);
      }
    } else if(!strcmp((char *) event.key, "DCI_aggregation_min") && event.value!=NULL && validate_mac(event)) {
      //Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      //mac_config = (Mac_config *) event.value;

      LOG_I(EMU,"DCI_aggregation_min update \n");

      if(event.ue == -1) {
        UE_list = &eNB_mac_inst->UE_list;
	/*
        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          oai_emulation->mac_config[i].DCI_aggregation_min=mac_config[i].DCI_aggregation_min;
          UE_list->UE_template[0][i].DCI_aggregation_min = oai_emulation->mac_config[i].DCI_aggregation_min;
          LOG_I(EMU,"DCI_aggregation_min UE %d : \n", i);
          LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_template[0][i].DCI_aggregation_min);
	  }*/
      } else {
	/*        oai_emulation->mac_config[event.ue].DCI_aggregation_min=mac_config[event.ue].DCI_aggregation_min;
        UE_list->UE_template[0][event.ue].DCI_aggregation_min = oai_emulation->mac_config[event.ue].DCI_aggregation_min;
        LOG_I(EMU,"DCI_aggregation_min UE %d : \n", event.ue);
        LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_template[0][event.ue].DCI_aggregation_min);*/
      }
    } else if(!strcmp((char *) event.key, "DLSCH_dci_size_bits") && event.value!=NULL && validate_mac(event)) {
      //Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      //mac_config = (Mac_config *) event.value;


      LOG_I(EMU,"DLSCH_dci_size_bits update \n");

      if(event.ue == -1) {
        UE_list = &eNB_mac_inst->UE_list;
	/*
        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          oai_emulation->mac_config[i].DLSCH_dci_size_bits=mac_config[i].DLSCH_dci_size_bits;
          UE_list->UE_template[0][i].DLSCH_dci_size_bits = oai_emulation->mac_config[i].DLSCH_dci_size_bits;
          LOG_I(EMU,"DLSCH_dci_size_bits UE %d: \n", i);
          LOG_I(EMU,"%" PRIu8,UE_list->UE_template[0][i].DLSCH_dci_size_bits);
	  }*/
      } else {
	/*        oai_emulation->mac_config[event.ue].DLSCH_dci_size_bits=mac_config[event.ue].DLSCH_dci_size_bits;
        UE_list->UE_template[0][event.ue].DLSCH_dci_size_bits = oai_emulation->mac_config[event.ue].DLSCH_dci_size_bits;
        LOG_I(EMU,"DLSCH_dci_size_bits UE %d: \n", event.ue);
        LOG_I(EMU,"%" PRIu8,UE_list->UE_template[0][event.ue].DLSCH_dci_size_bits);*/
      }

    } else if(!strcmp((char *) event.key, "ul_bandwidth") && event.value!=NULL) {


      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {


          for(j=0; j<MAX_NUM_LCID; j++) {

            if(&mac_config[i].ul_bandwidth[j]!=NULL) {
              oai_emulation->mac_config[i].ul_bandwidth[j]= mac_config[i].ul_bandwidth[j];
              UE_list->UE_sched_ctrl[i].ul_bandwidth[j] = oai_emulation->mac_config[i].ul_bandwidth[j];
              LOG_I(EMU,"ul_bandwidth UE %d LCID %d:",i,j);
              LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].ul_bandwidth[j]);
            }
          }

        }
      } else {
        oai_emulation->mac_config[event.ue].ul_bandwidth[event.lcid]= mac_config[event.ue].ul_bandwidth[event.lcid];
        UE_list->UE_sched_ctrl[event.ue].ul_bandwidth[event.lcid] = oai_emulation->mac_config[event.ue].ul_bandwidth[event.lcid];
        LOG_I(EMU,"ul_bandwidth UE %d LCID %d:",event.ue, event.lcid);
        LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[event.ue].ul_bandwidth[event.lcid]);
      }

    } else if(!strcmp((char *) event.key, "dl_bandwidth") && event.value!=NULL && validate_mac(event)) {

      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          if(event.value + i !=NULL) {

            for(j=0; j<MAX_NUM_LCID; j++) {

              if(&mac_config[i].dl_bandwidth[j]!=NULL) {
                oai_emulation->mac_config[i].dl_bandwidth[j]= mac_config[i].dl_bandwidth[j];
                UE_list->UE_sched_ctrl[i].dl_bandwidth[j] = oai_emulation->mac_config[i].dl_bandwidth[j];
                LOG_I(EMU,"dl_bandwidth UE %d LCID %d:",i,j);
                LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].dl_bandwidth[j]);
              }
            }
          }
        }
      } else {
        oai_emulation->mac_config[event.ue].dl_bandwidth[event.lcid]= mac_config[event.ue].dl_bandwidth[event.lcid];
        UE_list->UE_sched_ctrl[event.ue].dl_bandwidth[event.lcid] = oai_emulation->mac_config[event.ue].dl_bandwidth[event.lcid];
        LOG_I(EMU,"dl_bandwidth UE %d LCID %d:",event.ue, event.lcid);
        LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[event.ue].dl_bandwidth[event.lcid]);
      }
    } else if(!strcmp((char *) event.key, "min_ul_bandwidth") && event.value!=NULL) {


      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {


          for(j=0; j<MAX_NUM_LCID; j++) {

            if(&mac_config[i].min_ul_bandwidth[j]!=NULL) {
              oai_emulation->mac_config[i].min_ul_bandwidth[j]= mac_config[i].min_ul_bandwidth[j];
              UE_list->UE_sched_ctrl[i].min_ul_bandwidth[j] = oai_emulation->mac_config[i].min_ul_bandwidth[j];
              LOG_I(EMU,"min_ul_bandwidth UE %d LCID %d:",i,j);
              LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].min_ul_bandwidth[j]);
            }
          }

        }
      } else {
        oai_emulation->mac_config[event.ue].min_ul_bandwidth[event.lcid]= mac_config[event.ue].min_ul_bandwidth[event.lcid];
        UE_list->UE_sched_ctrl[event.ue].min_ul_bandwidth[event.lcid] = oai_emulation->mac_config[event.ue].min_ul_bandwidth[event.lcid];
        LOG_I(EMU,"min_ul_bandwidth UE %d LCID %d:",event.ue, event.lcid);
        LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[event.ue].min_ul_bandwidth[event.lcid]);
      }

    } else if(!strcmp((char *) event.key, "min_dl_bandwidth") && event.value!=NULL && validate_mac(event)) {

      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          if(event.value + i !=NULL) {

            for(j=0; j<MAX_NUM_LCID; j++) {

              if(&mac_config[i].min_dl_bandwidth[j]!=NULL) {
                oai_emulation->mac_config[i].min_dl_bandwidth[j]= mac_config[i].min_dl_bandwidth[j];
                UE_list->UE_sched_ctrl[i].min_dl_bandwidth[j] = oai_emulation->mac_config[i].min_dl_bandwidth[j];
                LOG_I(EMU,"min_dl_bandwidth UE %d LCID %d:",i,j);
                LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].min_dl_bandwidth[j]);
              }
            }
          }
        }
      } else {
        oai_emulation->mac_config[event.ue].min_dl_bandwidth[event.lcid]= mac_config[event.ue].min_dl_bandwidth[event.lcid];
        UE_list->UE_sched_ctrl[event.ue].min_dl_bandwidth[event.lcid] = oai_emulation->mac_config[event.ue].min_dl_bandwidth[event.lcid];
        LOG_I(EMU,"min_dl_bandwidth UE %d LCID %d:",event.ue, event.lcid);
        LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[event.ue].min_dl_bandwidth[event.lcid]);
      }
    } else if(!strcmp((char *) event.key, "ue_AggregatedMaximumBitrateDL") && event.value!=NULL && validate_mac(event)) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;

      LOG_I(EMU,"ue_AggregatedMaximumBitrateDL update \n");

      if(event.ue == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateDL= mac_config[i].ue_AggregatedMaximumBitrateDL;
          UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateDL = oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateDL;
          LOG_I(EMU,"ue_AggregatedMaximumBitrateDL UE %d:\n",event.ue);
          LOG_I(EMU,"%" PRIu64 "\n",UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateDL);
        }
      } else {
        oai_emulation->mac_config[event.ue].ue_AggregatedMaximumBitrateDL= mac_config[event.ue].ue_AggregatedMaximumBitrateDL;
        UE_list->UE_sched_ctrl[event.ue].ue_AggregatedMaximumBitrateDL = oai_emulation->mac_config[event.ue].ue_AggregatedMaximumBitrateDL;
        LOG_I(EMU,"ue_AggregatedMaximumBitrateDL UE %d:\n",event.ue);
        LOG_I(EMU,"%" PRIu64 "\n",UE_list->UE_sched_ctrl[event.ue].ue_AggregatedMaximumBitrateDL);
      }

    } else if(!strcmp((char *) event.key, "ue_AggregatedMaximumBitrateUL") && event.value!=NULL && validate_mac(event)) {

      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;

      LOG_I(EMU,"ue_AggregatedMaximumBitrateUL update \n");

      if(event.ue == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateUL= mac_config[i].ue_AggregatedMaximumBitrateUL;
          UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateUL = oai_emulation->mac_config[i].ue_AggregatedMaximumBitrateUL;
          LOG_I(EMU,"ue_AggregatedMaximumBitrateUL UE %d:\n",i);
          LOG_I(EMU,"%" PRIu64 "\n",UE_list->UE_sched_ctrl[i].ue_AggregatedMaximumBitrateUL);
        }
      } else {
        oai_emulation->mac_config[event.ue].ue_AggregatedMaximumBitrateUL= mac_config[event.ue].ue_AggregatedMaximumBitrateUL;
        UE_list->UE_sched_ctrl[event.ue].ue_AggregatedMaximumBitrateUL = oai_emulation->mac_config[event.ue].ue_AggregatedMaximumBitrateUL;
        LOG_I(EMU,"ue_AggregatedMaximumBitrateUL UE %d:\n",event.ue);
        LOG_I(EMU,"%" PRIu64 "\n",UE_list->UE_sched_ctrl[event.ue].ue_AggregatedMaximumBitrateUL);
      }

    } else if(!strcmp((char *) event.key, "cqiSchedInterval") && event.value!=NULL && validate_mac(event)) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;

      LOG_I(EMU,"cqiSchedInterval update \n");

      if(event.ue == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          oai_emulation->mac_config[i].cqiSchedInterval= mac_config[i].cqiSchedInterval;
          UE_list->UE_sched_ctrl[i].cqiSchedInterval = oai_emulation->mac_config[i].cqiSchedInterval;
          LOG_I(EMU,"cqiSchedInterval UE %d:\n", i);
          LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[i].cqiSchedInterval);
        }
      } else {
        oai_emulation->mac_config[event.ue].cqiSchedInterval= mac_config[event.ue].cqiSchedInterval;
        UE_list->UE_sched_ctrl[event.ue].cqiSchedInterval = oai_emulation->mac_config[event.ue].cqiSchedInterval;
        LOG_I(EMU,"cqiSchedInterval UE %d:\n", event.ue);
        LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[event.ue].cqiSchedInterval);
      }
    } else if(!strcmp((char *) event.key, "mac_ContentionResolutionTimer") && event.value!=NULL && validate_mac(event)) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      LOG_I(EMU,"mac_ContentionResolutionTimer update \n");

      if(event.ue == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          oai_emulation->mac_config[i].mac_ContentionResolutionTimer= mac_config[i].mac_ContentionResolutionTimer;
          UE_list->UE_sched_ctrl[i].mac_ContentionResolutionTimer = oai_emulation->mac_config[i].mac_ContentionResolutionTimer;
          LOG_I(EMU,"mac_ContentionResolutionTimer UE %d:\n", i);
          LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[i].mac_ContentionResolutionTimer);
        }
      } else {
        oai_emulation->mac_config[event.ue].mac_ContentionResolutionTimer= mac_config[event.ue].mac_ContentionResolutionTimer;
        UE_list->UE_sched_ctrl[event.ue].mac_ContentionResolutionTimer = oai_emulation->mac_config[event.ue].mac_ContentionResolutionTimer;
        LOG_I(EMU,"mac_ContentionResolutionTimer UE %d:\n", event.ue);
        LOG_I(EMU,"%" PRIu8 "\n",UE_list->UE_sched_ctrl[event.ue].mac_ContentionResolutionTimer);
      }
    } else if(!strcmp((char *) event.key, "max_allowed_rbs") && event.value!=NULL && validate_mac(event)) {


      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {
          if(event.value + i !=NULL) {

            for(j=0; j<MAX_NUM_LCID; j++) {

              if(&mac_config[i].max_allowed_rbs[j]!=NULL) {
                oai_emulation->mac_config[i].max_allowed_rbs[j]= mac_config[i].max_allowed_rbs[j];
                UE_list->UE_sched_ctrl[i].max_rbs_allowed_slice[j][0] = oai_emulation->mac_config[i].max_allowed_rbs[j];
                LOG_I(EMU,"max_allowed_rbs UE %d LCID %d:",i,j);
                LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[i].max_rbs_allowed_slice[j][0]);
              }
            }
          }
        }
      } else {
        oai_emulation->mac_config[event.ue].max_allowed_rbs[event.lcid]= mac_config[event.ue].max_allowed_rbs[event.lcid];
        UE_list->UE_sched_ctrl[event.ue].max_rbs_allowed_slice[event.lcid][0] = oai_emulation->mac_config[event.ue].max_allowed_rbs[event.lcid];
        LOG_I(EMU,"max_allowed_rbs UE %d LCID %d:",event.ue,event.lcid);
        LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[event.ue].max_rbs_allowed_slice[event.lcid][0]);
      }

    } else if(!strcmp((char *) event.key, "max_mcs") && event.value!=NULL && validate_mac(event)) {

      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        UE_list = &eNB_mac_inst->UE_list;

        for(i=UE_list->head; i>=0; i=UE_list->next[i]) {

          for(j=0; j<MAX_NUM_LCID; j++) {

            if(&mac_config[i].max_mcs[j]!=NULL) {
              oai_emulation->mac_config[i].max_mcs[j]= mac_config[i].max_mcs[j];
              UE_list->UE_sched_ctrl[i].max_mcs[j] = oai_emulation->mac_config[i].max_mcs[j];
              LOG_I(EMU,"max_mcs UE %d LCID %d:",i,j);
              LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[i].max_mcs[j]);
            }
          }

        }
      } else {
        oai_emulation->mac_config[event.ue].max_mcs[event.lcid]= mac_config[event.ue].max_mcs[event.lcid];
        UE_list->UE_sched_ctrl[event.ue].max_mcs[event.lcid] = oai_emulation->mac_config[event.ue].max_mcs[event.lcid];
        LOG_I(EMU,"max_mcs UE %d LCID %d:",event.ue,event.lcid);
        LOG_I(EMU,"%" PRIu16 "\n",UE_list->UE_sched_ctrl[event.ue].max_mcs[event.lcid]);
      }
    }
  }


}

void update_oai_model(char * key, void * value)
{

}

void update_sys_model(Event_t event)
{


  printf("\n\n\nA NEW SYS MODEL\n\n\n");
  OAI_Emulation *oai_emulation;
  oai_emulation = get_OAI_emulation();
  int i=0;

  if(event.optype == READ) {
    if(event.key == NULL) {
      LOG_I(EMU,"wall_penetration_loss_dB %G\n",oai_emulation->environment_system_config.wall_penetration_loss_dB);
      LOG_I(EMU,"system_bandwidth_MB %G\n",oai_emulation->environment_system_config.system_bandwidth_MB);
      LOG_I(EMU,"system_frequency_GHz %G\n",oai_emulation->environment_system_config.system_frequency_GHz);

      //Fading
      LOG_I(EMU,"fading.large_scale.selected_option %s\n",oai_emulation->environment_system_config.fading.large_scale.selected_option);
      LOG_I(EMU,"fading.large_scale.free_space %d\n",oai_emulation->environment_system_config.fading.large_scale.free_space);
      LOG_I(EMU,"fading.large_scale.urban %d\n",oai_emulation->environment_system_config.fading.large_scale.urban);
      LOG_I(EMU,"fading.large_scale.rural %d\n",oai_emulation->environment_system_config.fading.large_scale.rural);

      LOG_I(EMU,"fading.small_scale.selected_option %s\n",oai_emulation->environment_system_config.fading.small_scale.selected_option);
      LOG_I(EMU,"fading.small_scale.SCM_A %d\n",oai_emulation->environment_system_config.fading.small_scale.SCM_A);
      LOG_I(EMU,"fading.small_scale.SCM_B %d\n",oai_emulation->environment_system_config.fading.small_scale.SCM_B);
      LOG_I(EMU,"fading.small_scale.SCM_C %d\n",oai_emulation->environment_system_config.fading.small_scale.SCM_C);
      LOG_I(EMU,"fading.small_scale.SCM_D %d\n",oai_emulation->environment_system_config.fading.small_scale.SCM_D);
      LOG_I(EMU,"fading.small_scale.rayleigh_8tap %d\n",oai_emulation->environment_system_config.fading.small_scale.rayleigh_8tap);
      LOG_I(EMU,"fading.small_scale.EPA %d\n",oai_emulation->environment_system_config.fading.small_scale.EPA);
      LOG_I(EMU,"fading.small_scale.EVA %d\n",oai_emulation->environment_system_config.fading.small_scale.EVA);
      LOG_I(EMU,"fading.small_scale.ETU %d\n",oai_emulation->environment_system_config.fading.small_scale.ETU);

      LOG_I(EMU,"shadowing.decorrelation_distance_m %G\n",oai_emulation->environment_system_config.fading.shadowing.decorrelation_distance_m);
      LOG_I(EMU,"shadowing.variance_dB %G\n",oai_emulation->environment_system_config.fading.shadowing.variance_dB);
      LOG_I(EMU,"shadowing.inter_site_correlation %G\n",oai_emulation->environment_system_config.fading.shadowing.inter_site_correlation);

      LOG_I(EMU,"free_space_model_parameters.pathloss_exponent %G\n",oai_emulation->environment_system_config.fading.free_space_model_parameters.pathloss_exponent);
      LOG_I(EMU,"free_space_model_parameters.pathloss_0_dB %G\n",oai_emulation->environment_system_config.fading.free_space_model_parameters.pathloss_0_dB);

      LOG_I(EMU,"ricean_8tap.rice_factor_dB %d\n",oai_emulation->environment_system_config.fading.ricean_8tap.rice_factor_dB);

      //Antenna
      LOG_I(EMU,"antenna.eNB_antenna.number_of_sectors %d\n",oai_emulation->environment_system_config.antenna.eNB_antenna.number_of_sectors);
      LOG_I(EMU,"antenna.eNB_antenna.beam_width_dB %G\n",oai_emulation->environment_system_config.antenna.eNB_antenna.beam_width_dB);
      LOG_I(EMU,"antenna.eNB_antenna.antenna_gain_dBi %G\n",oai_emulation->environment_system_config.antenna.eNB_antenna.antenna_gain_dBi);
      LOG_I(EMU,"antenna.eNB_antenna.tx_power_dBm %G\n",oai_emulation->environment_system_config.antenna.eNB_antenna.tx_power_dBm);
      LOG_I(EMU,"antenna.eNB_antenna.rx_noise_level_dB %G\n",oai_emulation->environment_system_config.antenna.eNB_antenna.rx_noise_level_dB);

      for (i = 0; i<3; i++) {
        LOG_I(EMU,"antenna.eNB_antenna.alpha_rad %d %G\n",i,oai_emulation->environment_system_config.antenna.eNB_antenna.alpha_rad[i]);
      }

      for (i = 0; i<3; i++) {
        LOG_I(EMU,"antenna.eNB_antenna.antenna_orientation_degree %d %G\n",i,oai_emulation->environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[i]);
      }

      LOG_I(EMU,"antenna.UE_antenna.antenna_gain_dBi %G\n",oai_emulation->environment_system_config.antenna.UE_antenna.antenna_gain_dBi);
      LOG_I(EMU,"antenna.UE_antenna.tx_power_dBm %G\n",oai_emulation->environment_system_config.antenna.UE_antenna.tx_power_dBm);
      LOG_I(EMU,"antenna.UE_antenna.rx_noise_level_dB %G\n",oai_emulation->environment_system_config.antenna.UE_antenna.rx_noise_level_dB);
    } else if(!strcmp((char *) event.key, "wall_penetration_loss_dB")) {
      LOG_I(EMU,"wall_penetration_loss_dB %G\n",oai_emulation->environment_system_config.wall_penetration_loss_dB);
    } else if(!strcmp((char *) event.key, "system_bandwidth_MB")) {
      LOG_I(EMU,"system_bandwidth_MB %G\n",oai_emulation->environment_system_config.system_bandwidth_MB);
    } else if(!strcmp((char *) event.key, "system_frequency_GHz")) {
      LOG_I(EMU,"system_frequency_GHz %G\n",oai_emulation->environment_system_config.system_frequency_GHz);
    } else if(!strcmp((char *) event.key, "fading.large_scale.selected_option")) {
      LOG_I(EMU,"fading.large_scale.selected_option %s\n",oai_emulation->environment_system_config.fading.large_scale.selected_option);
    } else if(!strcmp((char *) event.key, "fading.large_scale.free_space")) {
      LOG_I(EMU,"fading.large_scale.free_space %d\n",oai_emulation->environment_system_config.fading.large_scale.free_space);
    } else if(!strcmp((char *) event.key, "fading.large_scale.urban")) {
      LOG_I(EMU,"fading.large_scale.urban %d\n",oai_emulation->environment_system_config.fading.large_scale.urban);
    } else if(!strcmp((char *) event.key, "fading.large_scale.rural")) {
      LOG_I(EMU,"fading.large_scale.rural %d\n",oai_emulation->environment_system_config.fading.large_scale.rural);
    } else if(!strcmp((char *) event.key, "fading.small_scale.selected_option")) {
      LOG_I(EMU,"fading.small_scale.selected_option %s\n",oai_emulation->environment_system_config.fading.small_scale.selected_option);
    } else if(!strcmp((char *) event.key, "fading.small_scale.SCM_A")) {
      LOG_I(EMU,"fading.small_scale.SCM_A %d\n",oai_emulation->environment_system_config.fading.small_scale.SCM_A);
    } else if(!strcmp((char *) event.key, "fading.small_scale.SCM_B")) {
      LOG_I(EMU,"fading.small_scale.SCM_B %d\n",oai_emulation->environment_system_config.fading.small_scale.SCM_B);
    } else if(!strcmp((char *) event.key, "fading.small_scale.SCM_C")) {
      LOG_I(EMU,"fading.small_scale.SCM_C %d\n",oai_emulation->environment_system_config.fading.small_scale.SCM_C);
    } else if(!strcmp((char *) event.key, "fading.small_scale.SCM_D")) {
      LOG_I(EMU,"fading.small_scale.SCM_D %d\n",oai_emulation->environment_system_config.fading.small_scale.SCM_D);
    } else if(!strcmp((char *) event.key, "fading.small_scale.rayleigh_8tap")) {
      LOG_I(EMU,"fading.small_scale.rayleigh_8tap %d\n",oai_emulation->environment_system_config.fading.small_scale.rayleigh_8tap);
    } else if(!strcmp((char *) event.key, "fading.small_scale.EPA")) {
      LOG_I(EMU,"fading.small_scale.EPA %d\n",oai_emulation->environment_system_config.fading.small_scale.EPA);
    } else if(!strcmp((char *) event.key, "fading.small_scale.EVA")) {
      LOG_I(EMU,"fading.small_scale.EVA %d\n",oai_emulation->environment_system_config.fading.small_scale.EVA);
    } else if(!strcmp((char *) event.key, "fading.small_scale.ETU")) {
      LOG_I(EMU,"fading.small_scale.ETU %d\n",oai_emulation->environment_system_config.fading.small_scale.ETU);
    } else if(!strcmp((char *) event.key, "shadowing.decorrelation_distance_m")) {
      LOG_I(EMU,"shadowing.decorrelation_distance_m %G\n",oai_emulation->environment_system_config.fading.shadowing.decorrelation_distance_m);
    } else if(!strcmp((char *) event.key, "shadowing.variance_dB")) {
      LOG_I(EMU,"shadowing.variance_dB %G\n",oai_emulation->environment_system_config.fading.shadowing.variance_dB);
    } else if(!strcmp((char *) event.key, "shadowing.inter_site_correlation")) {
      LOG_I(EMU,"shadowing.inter_site_correlation %G\n",oai_emulation->environment_system_config.fading.shadowing.inter_site_correlation);
    } else if(!strcmp((char *) event.key, "free_space_model_parameters.pathloss_exponent")) {
      LOG_I(EMU,"free_space_model_parameters.pathloss_exponent %G\n",oai_emulation->environment_system_config.fading.free_space_model_parameters.pathloss_exponent);
    } else if(!strcmp((char *) event.key, "free_space_model_parameters.pathloss_0_dB")) {
      LOG_I(EMU,"free_space_model_parameters.pathloss_0_dB %G\n",oai_emulation->environment_system_config.fading.free_space_model_parameters.pathloss_0_dB);
    } else if(!strcmp((char *) event.key, "ricean_8tap.rice_factor_dB")) {
      LOG_I(EMU,"ricean_8tap.rice_factor_dB %d\n",oai_emulation->environment_system_config.fading.ricean_8tap.rice_factor_dB);
    } else if(!strcmp((char *) event.key, "antenna.eNB_antenna.number_of_sectors")) {
      LOG_I(EMU,"antenna.eNB_antenna.number_of_sectors %d\n",oai_emulation->environment_system_config.antenna.eNB_antenna.number_of_sectors);
    } else if(!strcmp((char *) event.key, "antenna.eNB_antenna.beam_width_dB")) {
      LOG_I(EMU,"antenna.eNB_antenna.beam_width_dB %G\n",oai_emulation->environment_system_config.antenna.eNB_antenna.beam_width_dB);
    } else if(!strcmp((char *) event.key, "antenna.eNB_antenna.antenna_gain_dBi")) {
      LOG_I(EMU,"antenna.eNB_antenna.antenna_gain_dBi %G\n",oai_emulation->environment_system_config.antenna.eNB_antenna.antenna_gain_dBi);
    } else if(!strcmp((char *) event.key, "antenna.eNB_antenna.tx_power_dBm")) {
      LOG_I(EMU,"antenna.eNB_antenna.tx_power_dBm %G\n",oai_emulation->environment_system_config.antenna.eNB_antenna.tx_power_dBm);
    } else if(!strcmp((char *) event.key, "antenna.eNB_antenna.rx_noise_level_dB")) {
      LOG_I(EMU,"antenna.eNB_antenna.rx_noise_level_dB %G\n",oai_emulation->environment_system_config.antenna.eNB_antenna.rx_noise_level_dB);
    } else if(!strcmp((char *) event.key, "antenna.eNB_antenna.alpha_rad")) {
      for (i = 0; i<3; i++) {
        LOG_I(EMU,"antenna.eNB_antenna.alpha_rad %d %G\n",i,oai_emulation->environment_system_config.antenna.eNB_antenna.alpha_rad[i]);
      }
    } else if(!strcmp((char *) event.key, "antenna.eNB_antenna.antenna_orientation_degree")) {
      for (i = 0; i<3; i++) {
        LOG_I(EMU,"antenna.eNB_antenna.antenna_orientation_degree %d %G\n",i,oai_emulation->environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[i]);
      }
    } else if(!strcmp((char *) event.key, "antenna.UE_antenna.antenna_gain_dBi")) {
      LOG_I(EMU,"antenna.UE_antenna.antenna_gain_dBi %G\n",oai_emulation->environment_system_config.antenna.UE_antenna.antenna_gain_dBi);
    } else if(!strcmp((char *) event.key, "antenna.UE_antenna.tx_power_dBm")) {
      LOG_I(EMU,"antenna.UE_antenna.tx_power_dBm %G\n",oai_emulation->environment_system_config.antenna.UE_antenna.tx_power_dBm);
    } else if(!strcmp((char *) event.key, "antenna.UE_antenna.rx_noise_level_dB")) {
      LOG_I(EMU,"antenna.UE_antenna.rx_noise_level_dB %G\n",oai_emulation->environment_system_config.antenna.UE_antenna.rx_noise_level_dB);
    }
  } else if(event.optype == WRITE) {
    if(event.key == NULL && event.value!=NULL) {
      Environment_System_Config* new_env_config = (Environment_System_Config *) event.value;
      oai_emulation->environment_system_config.wall_penetration_loss_dB = new_env_config->wall_penetration_loss_dB;
      oai_emulation->environment_system_config.system_bandwidth_MB = new_env_config->system_bandwidth_MB;
      oai_emulation->environment_system_config.system_frequency_GHz = new_env_config->system_frequency_GHz;

      //Fading
      sprintf(oai_emulation->environment_system_config.fading.large_scale.selected_option,"%s",new_env_config->fading.large_scale.selected_option);
      oai_emulation->environment_system_config.fading.large_scale.free_space = new_env_config->fading.large_scale.free_space;
      oai_emulation->environment_system_config.fading.large_scale.urban = new_env_config->fading.large_scale.urban;
      oai_emulation->environment_system_config.fading.large_scale.rural = new_env_config->fading.large_scale.rural;

      sprintf(oai_emulation->environment_system_config.fading.small_scale.selected_option,"%s",new_env_config->fading.small_scale.selected_option);
      oai_emulation->environment_system_config.fading.small_scale.SCM_A = new_env_config->fading.small_scale.SCM_A;
      oai_emulation->environment_system_config.fading.small_scale.SCM_B = new_env_config->fading.small_scale.SCM_B;
      oai_emulation->environment_system_config.fading.small_scale.SCM_C = new_env_config->fading.small_scale.SCM_C;
      oai_emulation->environment_system_config.fading.small_scale.SCM_D = new_env_config->fading.small_scale.SCM_D;
      oai_emulation->environment_system_config.fading.small_scale.rayleigh_8tap = new_env_config->fading.small_scale.rayleigh_8tap;
      oai_emulation->environment_system_config.fading.small_scale.EPA = new_env_config->fading.small_scale.EPA;
      oai_emulation->environment_system_config.fading.small_scale.EVA = new_env_config->fading.small_scale.EVA;
      oai_emulation->environment_system_config.fading.small_scale.ETU = new_env_config->fading.small_scale.ETU;

      oai_emulation->environment_system_config.fading.shadowing.decorrelation_distance_m = new_env_config->fading.shadowing.decorrelation_distance_m;
      oai_emulation->environment_system_config.fading.shadowing.variance_dB = new_env_config->fading.shadowing.variance_dB;
      oai_emulation->environment_system_config.fading.shadowing.inter_site_correlation = new_env_config->fading.shadowing.inter_site_correlation;

      oai_emulation->environment_system_config.fading.free_space_model_parameters.pathloss_exponent = new_env_config->fading.free_space_model_parameters.pathloss_exponent;
      oai_emulation->environment_system_config.fading.free_space_model_parameters.pathloss_0_dB = new_env_config->fading.free_space_model_parameters.pathloss_0_dB;

      oai_emulation->environment_system_config.fading.ricean_8tap.rice_factor_dB = new_env_config->fading.ricean_8tap.rice_factor_dB;

      //Antenna
      oai_emulation->environment_system_config.antenna.eNB_antenna.number_of_sectors = new_env_config->antenna.eNB_antenna.number_of_sectors;
      oai_emulation->environment_system_config.antenna.eNB_antenna.beam_width_dB = new_env_config->antenna.eNB_antenna.beam_width_dB;
      oai_emulation->environment_system_config.antenna.eNB_antenna.antenna_gain_dBi = new_env_config->antenna.eNB_antenna.antenna_gain_dBi;
      oai_emulation->environment_system_config.antenna.eNB_antenna.tx_power_dBm = new_env_config->antenna.eNB_antenna.tx_power_dBm;
      oai_emulation->environment_system_config.antenna.eNB_antenna.rx_noise_level_dB = new_env_config->antenna.eNB_antenna.rx_noise_level_dB;

      for (i = 0; i<3; i++) {
        oai_emulation->environment_system_config.antenna.eNB_antenna.alpha_rad[i] = new_env_config->antenna.eNB_antenna.alpha_rad[i];
      }

      for (i = 0; i<3; i++) {
        oai_emulation->environment_system_config.antenna.eNB_antenna.antenna_orientation_degree[i] = new_env_config->antenna.eNB_antenna.antenna_orientation_degree[i];
      }

      oai_emulation->environment_system_config.antenna.UE_antenna.antenna_gain_dBi = new_env_config->antenna.UE_antenna.antenna_gain_dBi;
      oai_emulation->environment_system_config.antenna.UE_antenna.tx_power_dBm = new_env_config->antenna.UE_antenna.tx_power_dBm;
      oai_emulation->environment_system_config.antenna.UE_antenna.rx_noise_level_dB = new_env_config->antenna.UE_antenna.rx_noise_level_dB;
    }
  }
}

void update_topo_model(Event_t event)
{

  printf("\n\n\nA NEW MOB MODEL\n\n\n");
  OAI_Emulation *oai_emulation;
  oai_emulation = get_OAI_emulation();

  if(event.optype == READ) {
    if(event.key == NULL) {
      //read all values
      LOG_I(EMU,"UE Mobility type %s\n",oai_emulation->topology_config.mobility.UE_mobility.UE_mobility_type.selected_option);
      LOG_I(EMU,"eNB Mobility type %s\n",oai_emulation->topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option);
      LOG_I(EMU,"grid_trip_type %s\n",oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_trip_type.selected_option);
      LOG_I(EMU,"UE_moving_dynamics_min_speed_mps %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_speed_mps);
      LOG_I(EMU,"UE_moving_dynamics_min_sleep_ms %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_sleep_ms);
      LOG_I(EMU,"UE_moving_dynamics_max_sleep_ms %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_sleep_ms);
      LOG_I(EMU,"UE_moving_dynamics_max_speed_mps %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_speed_mps);
      LOG_I(EMU,"UE_moving_dynamics_min_journey_time_ms %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_journey_time_ms);
      LOG_I(EMU,"UE_moving_dynamics_max_journey_time_ms %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_journey_time_ms);
      LOG_I(EMU,"grid_map_horizontal_grid %d\n",oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_map.horizontal_grid);
      LOG_I(EMU,"grid_map_vertical_grid %d\n",oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_map.vertical_grid);
      LOG_I(EMU,"UE_initial_distribution_selected_option %s\n",oai_emulation->topology_config.mobility.UE_mobility.UE_initial_distribution.selected_option);
      LOG_I(EMU,"random_UE_distribution_number_of_nodes %d\n",oai_emulation->topology_config.mobility.UE_mobility.random_UE_distribution.number_of_nodes);
      LOG_I(EMU,"concentrated_UE_distribution_number_of_nodes %d\n",oai_emulation->topology_config.mobility.UE_mobility.concentrated_UE_distribution.number_of_nodes);
      LOG_I(EMU,"random_grid_number_of_nodes %d\n",oai_emulation->topology_config.mobility.UE_mobility.grid_UE_distribution.random_grid.number_of_nodes);
      LOG_I(EMU,"border_grid_number_of_nodes %d\n",oai_emulation->topology_config.mobility.UE_mobility.grid_UE_distribution.border_grid.number_of_nodes);
      LOG_I(EMU,"eNB_initial_distribution_selected_option %s\n",oai_emulation->topology_config.mobility.eNB_mobility.eNB_initial_distribution.selected_option);
      LOG_I(EMU,"random_eNB_distribution_number_of_cells %d\n",oai_emulation->topology_config.mobility.eNB_mobility.random_eNB_distribution.number_of_cells);
      LOG_I(EMU,"hexagonal_eNB_distribution_number_of_cells %d\n",oai_emulation->topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.number_of_cells);

      LOG_I(EMU,"hexagonal_eNB_distribution_inter_eNB_distance_km %G\n",oai_emulation->topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.inter_eNB_distance_km);
      LOG_I(EMU,"grid_eNB_distribution_number_of_grid_x %d\n",oai_emulation->topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_x);
      LOG_I(EMU,"grid_eNB_distribution_number_of_grid_y %d\n",oai_emulation->topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_y);

      LOG_I(EMU,"trace_config_trace_mobility_file %s\n",oai_emulation->topology_config.mobility.eNB_mobility.trace_config.trace_mobility_file);
      LOG_I(EMU,"trace_config_trace_mobility_file %s\n",oai_emulation->topology_config.mobility.UE_mobility.trace_config.trace_mobility_file);
      LOG_I(EMU,"sumo_config_command %s\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.command);
      LOG_I(EMU,"sumo_config_file %s\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.file);

      LOG_I(EMU,"sumo_config_start %d\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.start);
      LOG_I(EMU,"sumo_config_end %d\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.end);
      LOG_I(EMU,"sumo_config_step %d\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.step); //  1000ms
      LOG_I(EMU,"sumo_config_hport %d\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.hport);
    } else if(!strcmp((char *) event.key, "UE_mobility_type")) {
      LOG_I(EMU,"UE Mobility type %s\n",oai_emulation->topology_config.mobility.UE_mobility.UE_mobility_type.selected_option);
    } else if(!strcmp((char *) event.key, "eNB_mobility_type")) {
      LOG_I(EMU,"eNB Mobility type %s\n",oai_emulation->topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option);
    } else if(!strcmp((char *) event.key, "grid_trip_type")) {
      LOG_I(EMU,"grid_trip_type %s\n",oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_trip_type.selected_option);
    } else if(!strcmp((char *) event.key, "UE_moving_dynamics_min_speed_mps")) {
      LOG_I(EMU,"UE_moving_dynamics_min_speed_mps %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_speed_mps);
    } else if(!strcmp((char *) event.key, "UE_moving_dynamics_min_sleep_ms")) {
      LOG_I(EMU,"UE_moving_dynamics_min_sleep_ms %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_sleep_ms);
    } else if(!strcmp((char *) event.key, "UE_moving_dynamics_max_sleep_ms")) {
      LOG_I(EMU,"UE_moving_dynamics_max_sleep_ms %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_sleep_ms);
    } else if(!strcmp((char *) event.key, "UE_moving_dynamics_max_speed_mps")) {
      LOG_I(EMU,"UE_moving_dynamics_max_speed_mps %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_speed_mps);
    } else if(!strcmp((char *) event.key, "UE_moving_dynamics_min_journey_time_ms")) {
      LOG_I(EMU,"UE_moving_dynamics_min_journey_time_ms %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_journey_time_ms);
    } else if(!strcmp((char *) event.key, "UE_moving_dynamics_max_journey_time_ms")) {
      LOG_I(EMU,"UE_moving_dynamics_max_journey_time_ms %G\n",oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_journey_time_ms);
    } else if(!strcmp((char *) event.key, "grid_map_horizontal_grid")) {
      LOG_I(EMU,"grid_map_horizontal_grid %d\n",oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_map.horizontal_grid);
    } else if(!strcmp((char *) event.key, "grid_map_vertical_grid")) {
      LOG_I(EMU,"grid_map_vertical_grid %d\n",oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_map.vertical_grid);
    } else if(!strcmp((char *) event.key, "UE_initial_distribution_selected_option")) {
      LOG_I(EMU,"UE_initial_distribution_selected_option %s\n",oai_emulation->topology_config.mobility.UE_mobility.UE_initial_distribution.selected_option);
    } else if(!strcmp((char *) event.key, "random_UE_distribution_number_of_nodes")) {
      LOG_I(EMU,"random_UE_distribution_number_of_nodes %d\n",oai_emulation->topology_config.mobility.UE_mobility.random_UE_distribution.number_of_nodes);
    } else if(!strcmp((char *) event.key, "concentrated_UE_distribution_number_of_nodes")) {
      LOG_I(EMU,"concentrated_UE_distribution_number_of_nodes %d\n",oai_emulation->topology_config.mobility.UE_mobility.concentrated_UE_distribution.number_of_nodes);
    } else if(!strcmp((char *) event.key, "random_grid_number_of_nodes")) {
      LOG_I(EMU,"random_grid_number_of_nodes %d\n",oai_emulation->topology_config.mobility.UE_mobility.grid_UE_distribution.random_grid.number_of_nodes);
    } else if(!strcmp((char *) event.key, "order_grid_number_of_nodes")) {
      LOG_I(EMU,"border_grid_number_of_nodes %d\n",oai_emulation->topology_config.mobility.UE_mobility.grid_UE_distribution.border_grid.number_of_nodes);
    } else if(!strcmp((char *) event.key, "eNB_initial_distribution_selected_option")) {
      LOG_I(EMU,"eNB_initial_distribution_selected_option %s\n",oai_emulation->topology_config.mobility.eNB_mobility.eNB_initial_distribution.selected_option);
    } else if(!strcmp((char *) event.key, "random_eNB_distribution_number_of_cells")) {
      LOG_I(EMU,"random_eNB_distribution_number_of_cells %d\n",oai_emulation->topology_config.mobility.eNB_mobility.random_eNB_distribution.number_of_cells);
    } else if(!strcmp((char *) event.key, "hexagonal_eNB_distribution_number_of_cells")) {
      LOG_I(EMU,"hexagonal_eNB_distribution_number_of_cells %d\n",oai_emulation->topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.number_of_cells);
    } else if(!strcmp((char *) event.key, "hexagonal_eNB_distribution_inter_eNB_distance_km")) {
      LOG_I(EMU,"hexagonal_eNB_distribution_inter_eNB_distance_km %G\n",oai_emulation->topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.inter_eNB_distance_km);
    } else if(!strcmp((char *) event.key, "grid_eNB_distribution_number_of_grid_x")) {
      LOG_I(EMU,"grid_eNB_distribution_number_of_grid_x %d\n",oai_emulation->topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_x);
    } else if(!strcmp((char *) event.key, "grid_eNB_distribution_number_of_grid_y")) {
      LOG_I(EMU,"grid_eNB_distribution_number_of_grid_y %d\n",oai_emulation->topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_y);
    } else if(!strcmp((char *) event.key, "eNB_mobility_trace_config_trace_mobility_file")) {
      LOG_I(EMU,"eNB_trace_config_trace_mobility_file %s\n",oai_emulation->topology_config.mobility.eNB_mobility.trace_config.trace_mobility_file);
    } else if(!strcmp((char *) event.key, "UE_mobility_trace_config_trace_mobility_file")) {
      LOG_I(EMU,"UE_trace_config_trace_mobility_file %s\n",oai_emulation->topology_config.mobility.UE_mobility.trace_config.trace_mobility_file);
    } else if(!strcmp((char *) event.key, "sumo_config_command")) {
      LOG_I(EMU,"sumo_config_command %s\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.command);
    } else if(!strcmp((char *) event.key, "sumo_config_file")) {
      LOG_I(EMU,"sumo_config_file %s\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.file);
    } else if(!strcmp((char *) event.key, "sumo_config_start")) {
      LOG_I(EMU,"sumo_config_start %d\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.start);
    } else if(!strcmp((char *) event.key, "sumo_config_end")) {
      LOG_I(EMU,"sumo_config_end %d\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.end);
    } else if(!strcmp((char *) event.key, "sumo_config_step")) {
      LOG_I(EMU,"sumo_config_step %d\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.step); //  1000ms
    } else if(!strcmp((char *) event.key, "sumo_config_hport")) {
      LOG_I(EMU,"sumo_config_hport %d\n",oai_emulation->topology_config.mobility.UE_mobility.sumo_config.hport);
    }
  } else if(event.optype == WRITE && event.value !=NULL) {
    Mobility * new_mobility_model = (Mobility *) event.value;

    if(event.key == NULL) {
      //global model update

      sprintf(oai_emulation->topology_config.mobility.UE_mobility.UE_mobility_type.selected_option, "%s", new_mobility_model->UE_mobility.UE_mobility_type.selected_option);
      oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_map.horizontal_grid = new_mobility_model->UE_mobility.grid_walk.grid_map.horizontal_grid;
      oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_map.vertical_grid = new_mobility_model->UE_mobility.grid_walk.grid_map.vertical_grid;
      sprintf(oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_trip_type.selected_option, "%s", new_mobility_model->UE_mobility.grid_walk.grid_trip_type.selected_option);
      sprintf(oai_emulation->topology_config.mobility.UE_mobility.UE_initial_distribution.selected_option, "%s", new_mobility_model->UE_mobility.UE_initial_distribution.selected_option);
      oai_emulation->topology_config.mobility.UE_mobility.random_UE_distribution.number_of_nodes = new_mobility_model->UE_mobility.random_UE_distribution.number_of_nodes;
      oai_emulation->topology_config.mobility.UE_mobility.concentrated_UE_distribution.number_of_nodes = new_mobility_model->UE_mobility.concentrated_UE_distribution.number_of_nodes;
      oai_emulation->topology_config.mobility.UE_mobility.grid_UE_distribution.random_grid.number_of_nodes = new_mobility_model->UE_mobility.grid_UE_distribution.random_grid.number_of_nodes;
      oai_emulation->topology_config.mobility.UE_mobility.grid_UE_distribution.border_grid.number_of_nodes = new_mobility_model->UE_mobility.grid_UE_distribution.border_grid.number_of_nodes;
      oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_speed_mps = new_mobility_model->UE_mobility.UE_moving_dynamics.min_speed_mps;
      oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_speed_mps = new_mobility_model->UE_mobility.UE_moving_dynamics.max_speed_mps;
      oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_sleep_ms = new_mobility_model->UE_mobility.UE_moving_dynamics.min_sleep_ms;
      oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_sleep_ms = new_mobility_model->UE_mobility.UE_moving_dynamics.max_sleep_ms;
      oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_journey_time_ms = new_mobility_model->UE_mobility.UE_moving_dynamics.min_journey_time_ms;
      oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_journey_time_ms = new_mobility_model->UE_mobility.UE_moving_dynamics.max_journey_time_ms;
      sprintf(oai_emulation->topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option, "%s", new_mobility_model->eNB_mobility.eNB_mobility_type.selected_option);
      sprintf(oai_emulation->topology_config.mobility.eNB_mobility.eNB_initial_distribution.selected_option, "%s", new_mobility_model->eNB_mobility.eNB_initial_distribution.selected_option);
      oai_emulation->topology_config.mobility.eNB_mobility.random_eNB_distribution.number_of_cells = new_mobility_model->eNB_mobility.random_eNB_distribution.number_of_cells;
      oai_emulation->topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.number_of_cells = new_mobility_model->eNB_mobility.hexagonal_eNB_distribution.number_of_cells;
      oai_emulation->topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.inter_eNB_distance_km = new_mobility_model->eNB_mobility.hexagonal_eNB_distribution.inter_eNB_distance_km;
      oai_emulation->topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_x = new_mobility_model->eNB_mobility.grid_eNB_distribution.number_of_grid_x;
      oai_emulation->topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_y = new_mobility_model->eNB_mobility.grid_eNB_distribution.number_of_grid_y;

      sprintf(oai_emulation->topology_config.mobility.eNB_mobility.trace_config.trace_mobility_file,"static_1enb.tr");
      sprintf(oai_emulation->topology_config.mobility.UE_mobility.trace_config.trace_mobility_file,"static_2ues.tr");
      sprintf(oai_emulation->topology_config.mobility.UE_mobility.sumo_config.command,"sumo");
      sprintf(oai_emulation->topology_config.mobility.UE_mobility.sumo_config.file,"%s/UTIL/OMG/SUMO/SCENARIOS/scen.sumo.cfg",getenv("OPENAIR2_DIR"));
      sprintf(oai_emulation->topology_config.mobility.UE_mobility.sumo_config.hip,"127.0.1.1");

      oai_emulation->topology_config.mobility.UE_mobility.sumo_config.start = new_mobility_model->UE_mobility.sumo_config.start;
      oai_emulation->topology_config.mobility.UE_mobility.sumo_config.end = new_mobility_model->UE_mobility.sumo_config.end;
      oai_emulation->topology_config.mobility.UE_mobility.sumo_config.step = new_mobility_model->UE_mobility.sumo_config.step; //  1000ms
      oai_emulation->topology_config.mobility.UE_mobility.sumo_config.hport = new_mobility_model->UE_mobility.sumo_config.hport;
    } else {
      // event.key is not NULL
      if(!strcmp((char *) event.key, "UE_mobility_type")) {
        sprintf(oai_emulation->topology_config.mobility.UE_mobility.UE_mobility_type.selected_option, "%s", new_mobility_model->UE_mobility.UE_mobility_type.selected_option);

      } else if(!strcmp((char *) event.key, "eNB_mobility_type")) {
        sprintf(oai_emulation->topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option, "%s", new_mobility_model->eNB_mobility.eNB_mobility_type.selected_option);
      } else if(!strcmp((char *) event.key, "grid_trip_type")) {
        sprintf(oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_trip_type.selected_option, "%s", new_mobility_model->UE_mobility.grid_walk.grid_trip_type.selected_option);
      } else if(!strcmp((char *) event.key, "UE_moving_dynamics_min_speed_mps")) {
        oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_speed_mps = new_mobility_model->UE_mobility.UE_moving_dynamics.min_speed_mps;
      } else if(!strcmp((char *) event.key, "UE_moving_dynamics_min_sleep_ms")) {
        oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_sleep_ms = new_mobility_model->UE_mobility.UE_moving_dynamics.min_sleep_ms;
      } else if(!strcmp((char *) event.key, "UE_moving_dynamics_max_sleep_ms")) {
        oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_sleep_ms = new_mobility_model->UE_mobility.UE_moving_dynamics.max_sleep_ms;
      } else if(!strcmp((char *) event.key, "UE_moving_dynamics_max_speed_mps")) {
        oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_speed_mps = new_mobility_model->UE_mobility.UE_moving_dynamics.max_speed_mps;
      } else if(!strcmp((char *) event.key, "UE_moving_dynamics_min_journey_time_ms")) {
        oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.min_journey_time_ms = new_mobility_model->UE_mobility.UE_moving_dynamics.min_journey_time_ms;
      } else if(!strcmp((char *) event.key, "UE_moving_dynamics_max_journey_time_ms")) {
        oai_emulation->topology_config.mobility.UE_mobility.UE_moving_dynamics.max_journey_time_ms = new_mobility_model->UE_mobility.UE_moving_dynamics.max_journey_time_ms;
      } else if(!strcmp((char *) event.key, "grid_map_horizontal_grid")) {
        oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_map.horizontal_grid = new_mobility_model->UE_mobility.grid_walk.grid_map.horizontal_grid;
      } else if(!strcmp((char *) event.key, "grid_map_vertical_grid")) {
        oai_emulation->topology_config.mobility.UE_mobility.grid_walk.grid_map.vertical_grid = new_mobility_model->UE_mobility.grid_walk.grid_map.vertical_grid;
      } else if(!strcmp((char *) event.key, "UE_initial_distribution_selected_option")) {
        sprintf(oai_emulation->topology_config.mobility.UE_mobility.UE_initial_distribution.selected_option, "%s", new_mobility_model->UE_mobility.UE_initial_distribution.selected_option);
      } else if(!strcmp((char *) event.key, "random_UE_distribution_number_of_nodes")) {
        oai_emulation->topology_config.mobility.UE_mobility.random_UE_distribution.number_of_nodes = new_mobility_model->UE_mobility.random_UE_distribution.number_of_nodes;
      } else if(!strcmp((char *) event.key, "concentrated_UE_distribution_number_of_nodes")) {
        oai_emulation->topology_config.mobility.UE_mobility.concentrated_UE_distribution.number_of_nodes = new_mobility_model->UE_mobility.concentrated_UE_distribution.number_of_nodes;
      } else if(!strcmp((char *) event.key, "random_grid_number_of_nodes")) {
        oai_emulation->topology_config.mobility.UE_mobility.grid_UE_distribution.random_grid.number_of_nodes = new_mobility_model->UE_mobility.grid_UE_distribution.random_grid.number_of_nodes;
      } else if(!strcmp((char *) event.key, "border_grid_number_of_nodes")) {
        oai_emulation->topology_config.mobility.UE_mobility.grid_UE_distribution.border_grid.number_of_nodes = new_mobility_model->UE_mobility.grid_UE_distribution.border_grid.number_of_nodes;
      } else if(!strcmp((char *) event.key, "eNB_initial_distribution_selected_option")) {
        sprintf(oai_emulation->topology_config.mobility.eNB_mobility.eNB_initial_distribution.selected_option, "%s", new_mobility_model->eNB_mobility.eNB_initial_distribution.selected_option);
      } else if(!strcmp((char *) event.key, "random_eNB_distribution_number_of_cells")) {
        oai_emulation->topology_config.mobility.eNB_mobility.random_eNB_distribution.number_of_cells = new_mobility_model->eNB_mobility.random_eNB_distribution.number_of_cells;
      } else if(!strcmp((char *) event.key, "hexagonal_eNB_distribution_number_of_cells")) {
        oai_emulation->topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.number_of_cells = new_mobility_model->eNB_mobility.hexagonal_eNB_distribution.number_of_cells;
      } else if(!strcmp((char *) event.key, "hexagonal_eNB_distribution_inter_eNB_distance_km")) {
        oai_emulation->topology_config.mobility.eNB_mobility.hexagonal_eNB_distribution.inter_eNB_distance_km = new_mobility_model->eNB_mobility.hexagonal_eNB_distribution.inter_eNB_distance_km;
      } else if(!strcmp((char *) event.key, "grid_eNB_distribution_number_of_grid_x")) {
        oai_emulation->topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_x = new_mobility_model->eNB_mobility.grid_eNB_distribution.number_of_grid_x;
      } else if(!strcmp((char *) event.key, "grid_eNB_distribution_number_of_grid_y")) {
        oai_emulation->topology_config.mobility.eNB_mobility.grid_eNB_distribution.number_of_grid_y = new_mobility_model->eNB_mobility.grid_eNB_distribution.number_of_grid_y;
      } else if(!strcmp((char *) event.key, "eNB_mobility_trace_config_trace_mobility_file")) {
        sprintf(oai_emulation->topology_config.mobility.eNB_mobility.trace_config.trace_mobility_file,"static_1enb.tr");
      } else if(!strcmp((char *) event.key, "UE_mobility_trace_config_trace_mobility_file")) {
        sprintf(oai_emulation->topology_config.mobility.UE_mobility.trace_config.trace_mobility_file,"static_2ues.tr");
      } else if(!strcmp((char *) event.key, "sumo_config_command")) {
        sprintf(oai_emulation->topology_config.mobility.UE_mobility.sumo_config.command,"sumo");
      } else if(!strcmp((char *) event.key, "sumo_config_file")) {
        snprintf( oai_emulation->topology_config.mobility.UE_mobility.sumo_config.file, 256, "%s/UTIL/OMG/SUMO/SCENARIOS/scen.sumo.cfg", getenv("OPENAIR2_DIR") );
      } else if(!strcmp((char *) event.key, "sumo_config_start")) {
        oai_emulation->topology_config.mobility.UE_mobility.sumo_config.start = new_mobility_model->UE_mobility.sumo_config.start;
      } else if(!strcmp((char *) event.key, "sumo_config_end")) {
        oai_emulation->topology_config.mobility.UE_mobility.sumo_config.end = new_mobility_model->UE_mobility.sumo_config.end;
      } else if(!strcmp((char *) event.key, "sumo_config_step")) {
        oai_emulation->topology_config.mobility.UE_mobility.sumo_config.step = new_mobility_model->UE_mobility.sumo_config.step;
      } else if(!strcmp((char *) event.key, "sumo_config_hport")) {
        oai_emulation->topology_config.mobility.UE_mobility.sumo_config.hport = new_mobility_model->UE_mobility.sumo_config.hport;
      } else if(!strcmp((char *) event.key, "sumo_config.hip")) {
        sprintf(oai_emulation->topology_config.mobility.UE_mobility.sumo_config.hip,"127.0.1.1");
      }
    }

  }

}

void update_app_model(Event_t event)
{


  printf("\n\n\nA NEW APP MODEL\n\n\n");
  OAI_Emulation *oai_emulation;
  oai_emulation = get_OAI_emulation();
  int i=0;

  if(event.optype == READ) {
    if(event.key == NULL) {
      //print all values
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"predefined_traffic.source_id %d: %s\n",i,oai_emulation->application_config.predefined_traffic.source_id[i]);
        LOG_I(EMU,"predefined_traffic.background %d %s\n",i,oai_emulation->application_config.predefined_traffic.background[i]);
        LOG_I(EMU,"predefined_traffic.aggregation_level %d %d\n",i,oai_emulation->application_config.predefined_traffic.aggregation_level[i]);

        if(oai_emulation->application_config.predefined_traffic.destination_id[i]!=NULL)
          LOG_I(EMU,"predefined_traffic.destination_id %d %s\n",i,oai_emulation->application_config.predefined_traffic.destination_id[i]);

        if(oai_emulation->application_config.customized_traffic.source_id[i]!=NULL)
          LOG_I(EMU,"customized_traffic.source_id %d %s\n",i,oai_emulation->application_config.customized_traffic.source_id[i]);

        if(oai_emulation->application_config.customized_traffic.destination_id[i]!=NULL)
          LOG_I(EMU,"customized_traffic.destination_id %d %s\n",i,oai_emulation->application_config.customized_traffic.destination_id[i]);

        if(oai_emulation->application_config.customized_traffic.transport_protocol[i]!=NULL)
          LOG_I(EMU,"customized_traffic.transport_protocol %d %s\n",i,oai_emulation->application_config.customized_traffic.transport_protocol[i]);

        if(oai_emulation->application_config.customized_traffic.background[i]!=NULL)
          LOG_I(EMU,"customized_traffic.background %d %s\n",i,oai_emulation->application_config.customized_traffic.background[i]);

        if(oai_emulation->application_config.customized_traffic.ip_version[i]!=NULL)
          LOG_I(EMU,"customized_traffic.ip_version %d %s\n",i,oai_emulation->application_config.customized_traffic.ip_version[i]);

        LOG_I(EMU,"customized_traffic.aggregation_level %d %d\n",i,oai_emulation->application_config.customized_traffic.aggregation_level[i]);

        if(oai_emulation->application_config.customized_traffic.idt_dist[i]!=NULL)
          LOG_I(EMU,"customized_traffic.idt_dist %d %s\n",i,oai_emulation->application_config.customized_traffic.idt_dist[i]);

        LOG_I(EMU,"customized_traffic.idt_min_ms %d %d\n",i,oai_emulation->application_config.customized_traffic.idt_min_ms[i]);
        LOG_I(EMU,"customized_traffic.idt_max_ms %d %d\n",i,oai_emulation->application_config.customized_traffic.idt_max_ms[i]);
        LOG_I(EMU,"customized_traffic.idt_standard_deviation %d %G\n",i,oai_emulation->application_config.customized_traffic.idt_standard_deviation[i]);
        LOG_I(EMU,"customized_traffic.idt_lambda %d %G\n",i,oai_emulation->application_config.customized_traffic.idt_lambda[i]);
        LOG_I(EMU,"customized_traffic.size_dist %d %s\n",i,oai_emulation->application_config.customized_traffic.size_dist[i]);
        LOG_I(EMU,"customized_traffic.size_min_byte %d %d\n",i,oai_emulation->application_config.customized_traffic.size_min_byte[i]);//
        LOG_I(EMU,"customized_traffic.size_max_byte %d %d\n",i,oai_emulation->application_config.customized_traffic.size_max_byte[i]);//
        LOG_I(EMU,"customized_traffic.size_standard_deviation %d %G\n",i,oai_emulation->application_config.customized_traffic.size_standard_deviation[i]);//
        LOG_I(EMU,"customized_traffic.size_lambda %d %G\n",i,oai_emulation->application_config.customized_traffic.size_lambda[i]);
        LOG_I(EMU,"ustomized_traffic.stream %d %d\n",i,oai_emulation->application_config.customized_traffic.stream[i]);
        LOG_I(EMU,"customized_traffic.destination_port %d %d\n",i,oai_emulation->application_config.customized_traffic.destination_port[i]);
        LOG_I(EMU,"customized_traffic.prob_off_pu %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_off_pu[i]);
        LOG_I(EMU,"customized_traffic.prob_off_ed %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_off_ed[i]);
        LOG_I(EMU,"customized_traffic.prob_off_pe %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_off_pe[i]);
        LOG_I(EMU,"customized_traffic.prob_pu_ed %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_pu_ed[i]);//
        LOG_I(EMU,"customized_traffic.prob_pu_pe %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_pu_pe[i]);
        LOG_I(EMU,"customized_traffic.prob_ed_pe %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_ed_pe[i]);
        LOG_I(EMU,"customized_traffic.prob_ed_pu %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_ed_pu[i]);
        LOG_I(EMU,"customized_traffic.holding_time_off_ed %d %d\n",i,oai_emulation->application_config.customized_traffic.holding_time_off_ed[i]);
        LOG_I(EMU,"customized_traffic.holding_time_off_pu %d %d\n",i,oai_emulation->application_config.customized_traffic.holding_time_off_pu[i]);
        LOG_I(EMU,"customized_traffic.holding_time_off_pe %d %d\n",i,oai_emulation->application_config.customized_traffic.holding_time_off_pe[i]);
        LOG_I(EMU,"customized_traffic.holding_time_pe_off %d %d\n",i,oai_emulation->application_config.customized_traffic.holding_time_pe_off[i]);
        LOG_I(EMU,"customized_traffic.pu_size_pkts %d %d\n",i,oai_emulation->application_config.customized_traffic.pu_size_pkts[i]);
        LOG_I(EMU,"customized_traffic.ed_size_pkts %d %d\n",i,oai_emulation->application_config.customized_traffic.ed_size_pkts[i]);

      }
    } else if(!strcmp((char *) event.key, "predefined_traffic.source_id")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"predefined_traffic.source_id %d: %s\n",i,oai_emulation->application_config.predefined_traffic.source_id[i]);
      }
    } else if(!strcmp((char *) event.key, "predefined_traffic.background")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"predefined_traffic.background %d %s\n",i,oai_emulation->application_config.predefined_traffic.background[i]);
      }
    } else if(!strcmp((char *) event.key, "predefined_traffic.aggregation_level")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"predefined_traffic.aggregation_level %d %d\n",i,oai_emulation->application_config.predefined_traffic.aggregation_level[i]);
      }
    } else if(!strcmp((char *) event.key, "predefined_traffic.destination_id")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.predefined_traffic.destination_id[i]!=NULL)
          LOG_I(EMU,"predefined_traffic.destination_id %d %s\n",i,oai_emulation->application_config.predefined_traffic.destination_id[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.source_id")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.source_id[i]!=NULL)
          LOG_I(EMU,"customized_traffic.source_id %d %s\n",i,oai_emulation->application_config.customized_traffic.source_id[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.destination_id")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.destination_id[i]!=NULL)
          LOG_I(EMU,"customized_traffic.destination_id %d %s\n",i,oai_emulation->application_config.customized_traffic.destination_id[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.transport_protocol")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.transport_protocol[i]!=NULL)
          LOG_I(EMU,"customized_traffic.transport_protocol %d %s\n",i,oai_emulation->application_config.customized_traffic.transport_protocol[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.background")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.background[i]!=NULL)
          LOG_I(EMU,"customized_traffic.background %d %s\n",i,oai_emulation->application_config.customized_traffic.background[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.ip_version")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.ip_version[i]!=NULL)
          LOG_I(EMU,"customized_traffic.ip_version %d %s\n",i,oai_emulation->application_config.customized_traffic.ip_version[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.aggregation_level")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.aggregation_level %d %d\n",i,oai_emulation->application_config.customized_traffic.aggregation_level[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_dist")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.idt_dist[i]!=NULL)
          LOG_I(EMU,"customized_traffic.idt_dist %d %s\n",i,oai_emulation->application_config.customized_traffic.idt_dist[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_min_ms")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.idt_min_ms %d %d\n",i,oai_emulation->application_config.customized_traffic.idt_min_ms[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_max_ms")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.idt_max_ms %d %d\n",i,oai_emulation->application_config.customized_traffic.idt_max_ms[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_standard_deviation")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.idt_standard_deviation %d %G\n",i,oai_emulation->application_config.customized_traffic.idt_standard_deviation[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_lambda")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.idt_lambda %d %G\n",i,oai_emulation->application_config.customized_traffic.idt_lambda[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_dist")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.size_dist %d %s\n",i,oai_emulation->application_config.customized_traffic.size_dist[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_min_byte")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.size_min_byte %d %d\n",i,oai_emulation->application_config.customized_traffic.size_min_byte[i]);//
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_min_byte")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.size_min_byte %d %d\n",i,oai_emulation->application_config.customized_traffic.size_min_byte[i]);//
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_max_byte")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.size_max_byte %d %d\n",i,oai_emulation->application_config.customized_traffic.size_max_byte[i]);//
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_standard_deviation")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.size_standard_deviation %d %G\n",i,oai_emulation->application_config.customized_traffic.size_standard_deviation[i]);//
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_lambda")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.size_lambda %d %G\n",i,oai_emulation->application_config.customized_traffic.size_lambda[i]);
      }
    } else if(!strcmp((char *) event.key, "ustomized_traffic.stream")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"ustomized_traffic.stream %d %d\n",i,oai_emulation->application_config.customized_traffic.stream[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.destination_port")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.destination_port %d %d\n",i,oai_emulation->application_config.customized_traffic.destination_port[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_off_pu")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.prob_off_pu %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_off_pu[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_off_ed")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.prob_off_ed %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_off_ed[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_off_pe")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.prob_off_pe %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_off_pe[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_pu_ed")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.prob_pu_ed %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_pu_ed[i]);//
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_pu_pe")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.prob_pu_pe %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_pu_pe[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_ed_pe")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.prob_ed_pe %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_ed_pe[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_ed_pu")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.prob_ed_pu %d %G\n",i,oai_emulation->application_config.customized_traffic.prob_ed_pu[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.holding_time_off_ed")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.holding_time_off_ed %d %d\n",i,oai_emulation->application_config.customized_traffic.holding_time_off_ed[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.holding_time_off_pu")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.holding_time_off_pu %d %d\n",i,oai_emulation->application_config.customized_traffic.holding_time_off_pu[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.holding_time_off_pe")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.holding_time_off_pe %d %d\n",i,oai_emulation->application_config.customized_traffic.holding_time_off_pe[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.holding_time_pe_off")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.holding_time_pe_off %d %d\n",i,oai_emulation->application_config.customized_traffic.holding_time_pe_off[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.pu_size_pkts")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.pu_size_pkts %d %d\n",i,oai_emulation->application_config.customized_traffic.pu_size_pkts[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.ed_size_pkts")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        LOG_I(EMU,"customized_traffic.ed_size_pkts %d %d\n",i,oai_emulation->application_config.customized_traffic.ed_size_pkts[i]);
      }
    }
  } else if(event.optype == WRITE && event.value!=NULL) {
    Application_Config * new_app_config = (Application_Config *) event.value;

    if(event.key == NULL) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        sprintf(oai_emulation->application_config.predefined_traffic.source_id[i],"%s",new_app_config->predefined_traffic.source_id[i]);
        // LOG_I(EMU,"predefined_traffic.application_type %d: %s\n",i,oai_emulation->application_config.predefined_traffic.application_type[i]);
        sprintf(oai_emulation->application_config.predefined_traffic.background[i],"%s",new_app_config->predefined_traffic.background[i]);
        oai_emulation->application_config.predefined_traffic.aggregation_level[i] = new_app_config->predefined_traffic.aggregation_level[i];

        if(oai_emulation->application_config.predefined_traffic.destination_id[i]!=NULL)
          sprintf(oai_emulation->application_config.predefined_traffic.destination_id[i],"%s",new_app_config->predefined_traffic.destination_id[i]);

        if(oai_emulation->application_config.customized_traffic.source_id[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.source_id[i],"%s",new_app_config->customized_traffic.source_id[i]);

        if(oai_emulation->application_config.customized_traffic.destination_id[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.destination_id[i],"%s",new_app_config->customized_traffic.destination_id[i]);

        if(oai_emulation->application_config.customized_traffic.transport_protocol[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.transport_protocol[i],"%s",new_app_config->customized_traffic.transport_protocol[i]);

        if(oai_emulation->application_config.customized_traffic.background[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.background[i],"%s",new_app_config->customized_traffic.background[i]);

        if(oai_emulation->application_config.customized_traffic.ip_version[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.ip_version[i],"%s",new_app_config->customized_traffic.ip_version[i]);

        oai_emulation->application_config.customized_traffic.aggregation_level[i]  = new_app_config->customized_traffic.aggregation_level[i];

        if(oai_emulation->application_config.customized_traffic.idt_dist[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.idt_dist[i],"%s",new_app_config->customized_traffic.idt_dist[i]);

        oai_emulation->application_config.customized_traffic.idt_min_ms[i] = new_app_config->customized_traffic.idt_min_ms[i];
        oai_emulation->application_config.customized_traffic.idt_max_ms[i] = new_app_config->customized_traffic.idt_max_ms[i];
        oai_emulation->application_config.customized_traffic.idt_standard_deviation[i] = new_app_config->customized_traffic.idt_standard_deviation[i];
        oai_emulation->application_config.customized_traffic.idt_lambda[i] = new_app_config->customized_traffic.idt_lambda[i];
        sprintf(oai_emulation->application_config.customized_traffic.size_dist[i],"%s",new_app_config->customized_traffic.size_dist[i]);
        oai_emulation->application_config.customized_traffic.size_min_byte[i] = new_app_config->customized_traffic.size_min_byte[i];//
        oai_emulation->application_config.customized_traffic.size_max_byte[i] = new_app_config->customized_traffic.size_max_byte[i];//
        oai_emulation->application_config.customized_traffic.size_standard_deviation[i] = new_app_config->customized_traffic.size_standard_deviation[i];//
        oai_emulation->application_config.customized_traffic.size_lambda[i] = new_app_config->customized_traffic.size_lambda[i];
        oai_emulation->application_config.customized_traffic.stream[i] = new_app_config->customized_traffic.stream[i];
        oai_emulation->application_config.customized_traffic.destination_port[i] = new_app_config->customized_traffic.destination_port[i];
        oai_emulation->application_config.customized_traffic.prob_off_pu[i] = new_app_config->customized_traffic.prob_off_pu[i];
        oai_emulation->application_config.customized_traffic.prob_off_ed[i] = new_app_config->customized_traffic.prob_off_ed[i];
        oai_emulation->application_config.customized_traffic.prob_off_pe[i] = new_app_config->customized_traffic.prob_off_pe[i];
        oai_emulation->application_config.customized_traffic.prob_pu_ed[i] = new_app_config->customized_traffic.prob_pu_ed[i];//
        oai_emulation->application_config.customized_traffic.prob_pu_pe[i] = new_app_config->customized_traffic.prob_pu_pe[i];
        oai_emulation->application_config.customized_traffic.prob_ed_pe[i] = new_app_config->customized_traffic.prob_ed_pe[i];
        oai_emulation->application_config.customized_traffic.prob_ed_pu[i] = new_app_config->customized_traffic.prob_ed_pu[i];
        oai_emulation->application_config.customized_traffic.holding_time_off_ed[i] = new_app_config->customized_traffic.holding_time_off_ed[i];
        oai_emulation->application_config.customized_traffic.holding_time_off_pu[i] = new_app_config->customized_traffic.holding_time_off_pu[i];
        oai_emulation->application_config.customized_traffic.holding_time_off_pe[i] = new_app_config->customized_traffic.holding_time_off_pe[i];
        oai_emulation->application_config.customized_traffic.holding_time_pe_off[i] = new_app_config->customized_traffic.holding_time_pe_off[i];
        oai_emulation->application_config.customized_traffic.pu_size_pkts[i] = new_app_config->customized_traffic.pu_size_pkts[i];
        oai_emulation->application_config.customized_traffic.ed_size_pkts[i] = new_app_config->customized_traffic.ed_size_pkts[i];

      }

    } else if(!strcmp((char *) event.key, "predefined_traffic.source_id")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        sprintf(oai_emulation->application_config.predefined_traffic.source_id[i],"%s",new_app_config->predefined_traffic.source_id[i]);
      }
    } else if(!strcmp((char *) event.key, "predefined_traffic.background")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        sprintf(oai_emulation->application_config.predefined_traffic.background[i],"%s",new_app_config->predefined_traffic.background[i]);
      }
    } else if(!strcmp((char *) event.key, "predefined_traffic.aggregation_level")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.predefined_traffic.aggregation_level[i] = new_app_config->predefined_traffic.aggregation_level[i];
      }
    } else if(!strcmp((char *) event.key, "predefined_traffic.destination_id")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.predefined_traffic.destination_id[i]!=NULL)
          sprintf(oai_emulation->application_config.predefined_traffic.destination_id[i],"%s",new_app_config->predefined_traffic.destination_id[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.source_id")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.source_id[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.source_id[i],"%s",new_app_config->customized_traffic.source_id[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.destination_id")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.destination_id[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.destination_id[i],"%s",new_app_config->customized_traffic.destination_id[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.transport_protocol")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.transport_protocol[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.transport_protocol[i],"%s",new_app_config->customized_traffic.transport_protocol[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.background")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.background[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.background[i],"%s",new_app_config->customized_traffic.background[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.ip_version")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.ip_version[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.ip_version[i],"%s",*((char **) event.value));
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.aggregation_level")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.aggregation_level[i]  = new_app_config->customized_traffic.aggregation_level[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_dist")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        if(oai_emulation->application_config.customized_traffic.idt_dist[i]!=NULL)
          sprintf(oai_emulation->application_config.customized_traffic.idt_dist[i],"%s",new_app_config->customized_traffic.idt_dist[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_min_ms")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.idt_min_ms[i] = new_app_config->customized_traffic.idt_min_ms[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_max_ms")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.idt_max_ms[i] = new_app_config->customized_traffic.idt_max_ms[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_standard_deviation")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.idt_standard_deviation[i] = new_app_config->customized_traffic.idt_standard_deviation[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.idt_lambda")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.idt_lambda[i] = new_app_config->customized_traffic.idt_lambda[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_dist")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        sprintf(oai_emulation->application_config.customized_traffic.size_dist[i],"%s",new_app_config->customized_traffic.size_dist[i]);
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_min_byte")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.size_min_byte[i] = new_app_config->customized_traffic.size_min_byte[i];//
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_max_byte")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.size_max_byte[i] = new_app_config->customized_traffic.size_max_byte[i];//
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_standard_deviation")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.size_standard_deviation[i] = new_app_config->customized_traffic.size_standard_deviation[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.size_lambda")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.size_lambda[i] = new_app_config->customized_traffic.size_lambda[i];
      }
    } else if(!strcmp((char *) event.key, "ustomized_traffic.stream")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.stream[i] = new_app_config->customized_traffic.stream[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.destination_port")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.destination_port[i] = new_app_config->customized_traffic.destination_port[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_off_pu")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.prob_off_pu[i] = new_app_config->customized_traffic.prob_off_pu[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_off_ed")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.prob_off_ed[i] = new_app_config->customized_traffic.prob_off_ed[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_off_pe")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.prob_off_pe[i] = new_app_config->customized_traffic.prob_off_pe[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_pu_ed")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.prob_pu_ed[i] = new_app_config->customized_traffic.prob_pu_ed[i];//
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_pu_pe")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.prob_pu_pe[i] = new_app_config->customized_traffic.prob_pu_pe[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_ed_pe")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.prob_ed_pe[i] = new_app_config->customized_traffic.prob_ed_pe[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.prob_ed_pu")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.prob_ed_pu[i] = new_app_config->customized_traffic.prob_ed_pu[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.holding_time_off_ed")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.holding_time_off_ed[i] = new_app_config->customized_traffic.holding_time_off_ed[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.holding_time_off_pu")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.holding_time_off_pu[i] = new_app_config->customized_traffic.holding_time_off_pu[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.holding_time_off_pe")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.holding_time_off_pe[i] = new_app_config->customized_traffic.holding_time_off_pe[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.holding_time_pe_off")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.holding_time_pe_off[i] = new_app_config->customized_traffic.holding_time_pe_off[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.pu_size_pkts")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.pu_size_pkts[i] = new_app_config->customized_traffic.pu_size_pkts[i];
      }
    } else if(!strcmp((char *) event.key, "customized_traffic.ed_size_pkts")) {
      for (i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++) {
        oai_emulation->application_config.customized_traffic.ed_size_pkts[i] = new_app_config->customized_traffic.ed_size_pkts[i];
      }
    }

  }
}

void update_emu_model(Event_t event)
{
  //    Emulation_Config emulation_config;  /*!< \brief Emulation configuration */


  printf("\n\n\nA NEW EMU MODEL\n\n\n");
  OAI_Emulation *oai_emulation;
  oai_emulation = get_OAI_emulation();

  if(event.optype == READ) {
    if(event.key == NULL) {
      LOG_I(EMU,"emulation_time_ms %G\n",oai_emulation->emulation_config.emulation_time_ms);
      LOG_I(EMU,"curve %s\n",oai_emulation->emulation_config.curve);
      LOG_I(EMU,"brackground_stats %s\n",oai_emulation->emulation_config.background_stats);

      //performance metrics
      LOG_I(EMU,"performance_metrics.throughput %s\n",oai_emulation->emulation_config.performance_metrics.throughput);
      LOG_I(EMU,"performance_metrics.latency %s\n",oai_emulation->emulation_config.performance_metrics.latency);
      LOG_I(EMU,"performance_metrics.loss_rate %s\n",oai_emulation->emulation_config.performance_metrics.loss_rate);
      LOG_I(EMU,"performance_metrics.owd_radio_access %s\n",oai_emulation->emulation_config.performance_metrics.owd_radio_access);
      LOG_I(EMU,"performance_metrics.signalling_overhead %d\n",oai_emulation->emulation_config.performance_metrics.signalling_overhead);

      //Layer
      LOG_I(EMU,"layer.phy %d\n",oai_emulation->emulation_config.layer.phy);
      LOG_I(EMU,"layer.mac %d\n",oai_emulation->emulation_config.layer.mac);
      LOG_I(EMU,"layer.rlc %d\n",oai_emulation->emulation_config.layer.rlc);
      LOG_I(EMU,"layer.rrc %d\n",oai_emulation->emulation_config.layer.rrc);
      LOG_I(EMU,"layer.pdcp %d\n",oai_emulation->emulation_config.layer.pdcp);
      LOG_I(EMU,"layer.omg %d\n",oai_emulation->emulation_config.layer.omg);
      LOG_I(EMU,"layer.emu %d\n",oai_emulation->emulation_config.layer.emu);
      LOG_I(EMU,"layer.otg %d\n",oai_emulation->emulation_config.layer.otg);

      //Log_emu
      LOG_I(EMU,"log_emu.level %s\n",oai_emulation->emulation_config.log_emu.level);
      LOG_I(EMU,"log_emu.verbosity %s\n",oai_emulation->emulation_config.log_emu.verbosity);
      LOG_I(EMU,"log_emu.interval %d\n",oai_emulation->emulation_config.log_emu.interval);

      //Packet_trace
      LOG_I(EMU,"packet_trace.enabled %d\n",oai_emulation->emulation_config.packet_trace.enabled);
      LOG_I(EMU,"packet_trace.start_time %u\n",oai_emulation->emulation_config.packet_trace.start_time);
      LOG_I(EMU,"packet_trace.end_time %u\n",oai_emulation->emulation_config.packet_trace.end_time);

      //Seed
      LOG_I(EMU,"seed.value %d\n",oai_emulation->emulation_config.seed.value);

    } else if (!strcmp((char *) event.key, "emulation_time_ms")) {
      LOG_I(EMU,"emulation_time_ms %G\n",oai_emulation->emulation_config.emulation_time_ms);
    } else if (!strcmp((char *) event.key, "curve")) {
      LOG_I(EMU,"curve %s\n",oai_emulation->emulation_config.curve);
    } else if (!strcmp((char *) event.key, "brackground_stats")) {
      LOG_I(EMU,"brackground_stats %s\n",oai_emulation->emulation_config.background_stats);
    } else if (!strcmp((char *) event.key, "performance_metrics.throughput")) {
      LOG_I(EMU,"performance_metrics.throughput %s\n",oai_emulation->emulation_config.performance_metrics.throughput);
    } else if (!strcmp((char *) event.key, "performance_metrics.latency")) {
      LOG_I(EMU,"performance_metrics.latency %s\n",oai_emulation->emulation_config.performance_metrics.latency);
    } else if (!strcmp((char *) event.key, "performance_metrics.loss_rate")) {
      LOG_I(EMU,"performance_metrics.loss_rate %s\n",oai_emulation->emulation_config.performance_metrics.loss_rate);
    } else if (!strcmp((char *) event.key, "performance_metrics.owd_radio_access")) {
      LOG_I(EMU,"performance_metrics.owd_radio_access %s\n",oai_emulation->emulation_config.performance_metrics.owd_radio_access);
    } else if (!strcmp((char *) event.key, "performance_metrics.signalling_overhead")) {
      LOG_I(EMU,"performance_metrics.signalling_overhead %d\n",oai_emulation->emulation_config.performance_metrics.signalling_overhead);
    } else if (!strcmp((char *) event.key, "layer.phy")) {
      LOG_I(EMU,"layer.phy %d\n",oai_emulation->emulation_config.layer.phy);
    } else if (!strcmp((char *) event.key, "layer.mac")) {
      LOG_I(EMU,"layer.mac %d\n",oai_emulation->emulation_config.layer.mac);
    } else if (!strcmp((char *) event.key, "layer.rlc")) {
      LOG_I(EMU,"layer.rlc %d\n",oai_emulation->emulation_config.layer.rlc);
    } else if (!strcmp((char *) event.key, "layer.rrc")) {
      LOG_I(EMU,"layer.rrc %d\n",oai_emulation->emulation_config.layer.rrc);
    } else if (!strcmp((char *) event.key, "layer.pdcp")) {
      LOG_I(EMU,"layer.pdcp %d\n",oai_emulation->emulation_config.layer.pdcp);
    } else if (!strcmp((char *) event.key, "layer.omg")) {
      LOG_I(EMU,"layer.omg %d\n",oai_emulation->emulation_config.layer.omg);
    } else if (!strcmp((char *) event.key, "layer.emu")) {
      LOG_I(EMU,"layer.emu %d\n",oai_emulation->emulation_config.layer.emu);
    } else if (!strcmp((char *) event.key, "layer.otg")) {
      LOG_I(EMU,"layer.otg %d\n",oai_emulation->emulation_config.layer.otg);
    } else if (!strcmp((char *) event.key, "log_emu.level")) {
      LOG_I(EMU,"log_emu.level %s\n",oai_emulation->emulation_config.log_emu.level);
    } else if (!strcmp((char *) event.key, "log_emu.verbosity")) {
      LOG_I(EMU,"log_emu.verbosity %s\n",oai_emulation->emulation_config.log_emu.verbosity);
    } else if (!strcmp((char *) event.key, "log_emu.interval")) {
      LOG_I(EMU,"log_emu.interval %d\n",oai_emulation->emulation_config.log_emu.interval);
    } else if (!strcmp((char *) event.key, "packet_trace.enabled")) {
      LOG_I(EMU,"packet_trace.enabled %d\n",oai_emulation->emulation_config.packet_trace.enabled);
    } else if (!strcmp((char *) event.key, "packet_trace.start_time")) {
      LOG_I(EMU,"packet_trace.start_time %u\n",oai_emulation->emulation_config.packet_trace.start_time);
    } else if (!strcmp((char *) event.key, "packet_trace.end_time")) {
      LOG_I(EMU,"packet_trace.end_time %u\n",oai_emulation->emulation_config.packet_trace.end_time);
    } else if (!strcmp((char *) event.key, "customized_traffic_ed_size_pkts")) {
      LOG_I(EMU,"seed.value %d\n",oai_emulation->emulation_config.seed.value);
    }
  } else if(event.optype == WRITE && event.value!=NULL) {
    Emulation_Config * new_emu_config = (Emulation_Config *) event.value;

    if(event.key==NULL) {
      //update all parameters

      oai_emulation->emulation_config.emulation_time_ms = new_emu_config->emulation_time_ms;
      sprintf(oai_emulation->emulation_config.curve,"%s",new_emu_config->curve);
      sprintf(oai_emulation->emulation_config.background_stats,"%s",new_emu_config->background_stats);

      //performance metrics
      sprintf(oai_emulation->emulation_config.performance_metrics.throughput,"%s",new_emu_config->performance_metrics.throughput);
      sprintf(oai_emulation->emulation_config.performance_metrics.latency,"%s",new_emu_config->performance_metrics.latency);
      sprintf(oai_emulation->emulation_config.performance_metrics.loss_rate,"%s",new_emu_config->performance_metrics.loss_rate);
      sprintf(oai_emulation->emulation_config.performance_metrics.owd_radio_access,"%s",new_emu_config->performance_metrics.owd_radio_access);
      oai_emulation->emulation_config.performance_metrics.signalling_overhead = new_emu_config->performance_metrics.signalling_overhead;

      //Layer
      oai_emulation->emulation_config.layer.phy = new_emu_config->layer.phy;
      oai_emulation->emulation_config.layer.mac = new_emu_config->layer.mac;
      oai_emulation->emulation_config.layer.rlc = new_emu_config->layer.rlc;
      oai_emulation->emulation_config.layer.rrc = new_emu_config->layer.rrc;
      oai_emulation->emulation_config.layer.pdcp = new_emu_config->layer.pdcp;
      oai_emulation->emulation_config.layer.omg = new_emu_config->layer.omg;
      oai_emulation->emulation_config.layer.emu = new_emu_config->layer.emu;
      oai_emulation->emulation_config.layer.otg = new_emu_config->layer.otg;

      //Log_emu
      sprintf(oai_emulation->emulation_config.log_emu.level,"%s",new_emu_config->log_emu.level);
      sprintf(oai_emulation->emulation_config.log_emu.verbosity,"%s",new_emu_config->log_emu.verbosity);
      oai_emulation->emulation_config.log_emu.interval = new_emu_config->log_emu.interval;

      //Packet_trace
      oai_emulation->emulation_config.packet_trace.enabled = new_emu_config->packet_trace.enabled ;
      oai_emulation->emulation_config.packet_trace.start_time = new_emu_config->packet_trace.start_time;
      oai_emulation->emulation_config.packet_trace.end_time = new_emu_config->packet_trace.end_time;

      //Seed
      oai_emulation->emulation_config.seed.value = new_emu_config->packet_trace.end_time;
    } else if(event.key != NULL) {

      if (!strcmp((char *) event.key, "emulation_time_ms")) {
        oai_emulation->emulation_config.emulation_time_ms = new_emu_config->emulation_time_ms;
      } else if (!strcmp((char *) event.key, "curve")) {
        sprintf(oai_emulation->emulation_config.curve,"%s",new_emu_config->curve);
      } else if (!strcmp((char *) event.key, "brackground_stats")) {
        sprintf(oai_emulation->emulation_config.background_stats,"%s",new_emu_config->background_stats);
      } else if (!strcmp((char *) event.key, "performance_metrics.throughput")) {
        sprintf(oai_emulation->emulation_config.performance_metrics.throughput,"%s",new_emu_config->performance_metrics.throughput);
      } else if (!strcmp((char *) event.key, "performance_metrics.latency")) {
        sprintf(oai_emulation->emulation_config.performance_metrics.latency,"%s",new_emu_config->performance_metrics.latency);
      } else if (!strcmp((char *) event.key, "performance_metrics.loss_rate")) {
        sprintf(oai_emulation->emulation_config.performance_metrics.loss_rate,"%s",new_emu_config->performance_metrics.loss_rate);
      } else if (!strcmp((char *) event.key, "performance_metrics.owd_radio_access")) {
        sprintf(oai_emulation->emulation_config.performance_metrics.owd_radio_access,"%s",new_emu_config->performance_metrics.owd_radio_access);
      } else if (!strcmp((char *) event.key, "performance_metrics.signalling_overhead")) {
        oai_emulation->emulation_config.performance_metrics.signalling_overhead = new_emu_config->performance_metrics.signalling_overhead;
      } else if (!strcmp((char *) event.key, "layer.phy")) {
        oai_emulation->emulation_config.layer.phy = new_emu_config->layer.phy;
      } else if (!strcmp((char *) event.key, "layer.mac")) {
        oai_emulation->emulation_config.layer.mac = new_emu_config->layer.mac;
      } else if (!strcmp((char *) event.key, "layer.rlc")) {
        oai_emulation->emulation_config.layer.rlc = new_emu_config->layer.rlc;
      } else if (!strcmp((char *) event.key, "layer.rrc")) {
        oai_emulation->emulation_config.layer.rrc = new_emu_config->layer.rrc;
      } else if (!strcmp((char *) event.key, "layer.pdcp")) {
        oai_emulation->emulation_config.layer.pdcp = new_emu_config->layer.pdcp;
      } else if (!strcmp((char *) event.key, "layer.omg")) {
        oai_emulation->emulation_config.layer.omg = new_emu_config->layer.omg;
      } else if (!strcmp((char *) event.key, "layer.emu")) {
        oai_emulation->emulation_config.layer.emu = new_emu_config->layer.emu;
      } else if (!strcmp((char *) event.key, "layer.otg")) {
        oai_emulation->emulation_config.layer.otg = new_emu_config->layer.otg;
      } else if (!strcmp((char *) event.key, "log_emu.level")) {
        sprintf(oai_emulation->emulation_config.log_emu.level,"%s",new_emu_config->log_emu.level);
      } else if (!strcmp((char *) event.key, "log_emu.verbosity")) {
        sprintf(oai_emulation->emulation_config.log_emu.verbosity,"%s",new_emu_config->log_emu.verbosity);
      } else if (!strcmp((char *) event.key, "log_emu.interval")) {
        oai_emulation->emulation_config.log_emu.interval = new_emu_config->log_emu.interval;
      } else if (!strcmp((char *) event.key, "packet_trace.enabled")) {
        oai_emulation->emulation_config.packet_trace.enabled = new_emu_config->packet_trace.enabled ;
      } else if (!strcmp((char *) event.key, "packet_trace.start_time")) {
        oai_emulation->emulation_config.packet_trace.start_time = new_emu_config->packet_trace.start_time;
      } else if (!strcmp((char *) event.key, "packet_trace.end_time")) {
        oai_emulation->emulation_config.packet_trace.end_time = new_emu_config->packet_trace.end_time;
      } else if (!strcmp((char *) event.key, "customized_traffic_ed_size_pkts")) {
        oai_emulation->emulation_config.seed.value = new_emu_config->packet_trace.end_time;
      }
    }
  }

}


int validate_mac(Event_t event)
{
  int i=0;

  if(event.key ==NULL && event.value!=NULL) {
    Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
    mac_config = (Mac_config *) event.value;

    if(event.ue == -1 && event.lcid == -1) {
      for(i=0; i<NUMBER_OF_UE_MAX; i++) {
        if(mac_config[i].DCI_aggregation_min<=0) {
          return 0;
        }

        if(mac_config[i].DLSCH_dci_size_bits <=0) {
          return 0;
        }

        if(mac_config[i].priority !=NULL) {

          int j;

          for(j=0; j<MAX_NUM_LCID; j++) {

            if(mac_config[i].priority[j]<=0) {
              return 0;
            }
          }
        }

        if(&mac_config[i].ul_bandwidth !=NULL) {
          // faire boucle par ue puis par lcid

          int j=0;

          for(j=0; j<MAX_NUM_LCID; j++) {
            if(!(mac_config[i].ul_bandwidth[j]==6 || mac_config[i].ul_bandwidth[j]==15 || mac_config[i].ul_bandwidth[j]==25 || mac_config[i].ul_bandwidth[j]==50 || mac_config[i].ul_bandwidth[j]==75
                 || mac_config[i].ul_bandwidth[j]==100))
              return 0;
          }
        }

        if(&mac_config[i].dl_bandwidth !=NULL) {
          int j=0;

          for(j=0; j<MAX_NUM_LCID; j++) {
            if(!(mac_config[i].dl_bandwidth[j]==6 || mac_config[i].dl_bandwidth[j]==15 || mac_config[i].dl_bandwidth[j]==25 || mac_config[i].dl_bandwidth[j]==50 || mac_config[i].dl_bandwidth[j]==75
                 || mac_config[i].dl_bandwidth[j]==100))
              return 0;
          }
        }

        if(&mac_config[i].min_ul_bandwidth !=NULL) {
          // faire boucle par ue puis par lcid

          int j=0;

          for(j=0; j<MAX_NUM_LCID; j++) {
            if(!(mac_config[i].min_ul_bandwidth[j]==6 || mac_config[i].min_ul_bandwidth[j]==15 || mac_config[i].min_ul_bandwidth[j]==25 || mac_config[i].min_ul_bandwidth[j]==50
                 || mac_config[i].min_ul_bandwidth[j]==75 || mac_config[i].min_ul_bandwidth[j]==100))
              return 0;
          }
        }

        if(&mac_config[i].min_dl_bandwidth !=NULL) {
          int j=0;

          for(j=0; j<MAX_NUM_LCID; j++) {
            if(!(mac_config[i].min_dl_bandwidth[j]==6 || mac_config[i].min_dl_bandwidth[j]==15 || mac_config[i].min_dl_bandwidth[j]==25 || mac_config[i].min_dl_bandwidth[j]==50
                 || mac_config[i].min_dl_bandwidth[j]==75 || mac_config[i].min_dl_bandwidth[j]==100))
              return 0;
          }
        }

        if(&mac_config[i].ue_AggregatedMaximumBitrateDL !=NULL) {
          if(!(mac_config[i].ue_AggregatedMaximumBitrateDL>0 && mac_config[i].ue_AggregatedMaximumBitrateDL<10000000000))
            return 0;
        }

        if(&mac_config[i].ue_AggregatedMaximumBitrateUL !=NULL) {
          if(!(mac_config[i].ue_AggregatedMaximumBitrateUL>0 && mac_config[i].ue_AggregatedMaximumBitrateUL<10000000000))
            return 0;
        }

        if(&mac_config[i].cqiSchedInterval !=NULL ) {
          if(!(mac_config[i].cqiSchedInterval==1 || mac_config[i].cqiSchedInterval==2 || mac_config[i].cqiSchedInterval==5 || mac_config[i].cqiSchedInterval==10 || mac_config[i].cqiSchedInterval==20
               || mac_config[i].cqiSchedInterval==32 || mac_config[i].cqiSchedInterval==40 || mac_config[i].cqiSchedInterval==64 || mac_config[i].cqiSchedInterval==80 || mac_config[i].cqiSchedInterval==128
               || mac_config[i].cqiSchedInterval==160))
            return 0;
        }

        if(&mac_config[i].mac_ContentionResolutionTimer !=NULL) {
          if(!(mac_config[i].mac_ContentionResolutionTimer==8 || mac_config[i].mac_ContentionResolutionTimer==16 || mac_config[i].mac_ContentionResolutionTimer==24
               || mac_config[i].mac_ContentionResolutionTimer==32 || mac_config[i].mac_ContentionResolutionTimer==40 || mac_config[i].mac_ContentionResolutionTimer==48
               || mac_config[i].mac_ContentionResolutionTimer==56 || mac_config[i].mac_ContentionResolutionTimer==64))
            return 0;
        }

        if(mac_config->max_allowed_rbs !=NULL) {
          int j=0;

          for(j=0; j<MAX_NUM_LCID; j++) {
            if(mac_config[i].max_allowed_rbs[j]<=0)
              return 0;
          }
        }

        if(mac_config[i].max_mcs !=NULL) {
          int j=0;

          for(j=0; j<MAX_NUM_LCID; j++) {
            if(!(mac_config[i].max_mcs[j]>=0 && mac_config[i].max_mcs[j]<=27))
              return 0;
          }

        }
      }
    }
  }

  if(event.key==NULL && event.value!=NULL && event.ue!=-1 && event.lcid!=-1) {
    //check one param

    Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
    mac_config = (Mac_config *) event.value;

    i = event.ue;
    int j = event.lcid;

    if(&mac_config[i].DCI_aggregation_min) {
      if(mac_config[i].DCI_aggregation_min<=0) {
        return 0;
      }

    }

    if(&mac_config[i].DLSCH_dci_size_bits !=NULL) {
      if(mac_config[i].DLSCH_dci_size_bits <=0) {
        return 0;
      }
    }

    if(mac_config[i].priority !=NULL) {
      if(&mac_config[i].priority[j]!=NULL) {
        if(mac_config[i].priority[j]<=0) {
          return 0;
        }
      }

    }

    if(&mac_config[i].ul_bandwidth !=NULL) {

      if(!(mac_config[i].ul_bandwidth[j]==6 || mac_config[i].ul_bandwidth[j]==15 || mac_config[i].ul_bandwidth[j]==25 || mac_config[i].ul_bandwidth[j]==50 || mac_config[i].ul_bandwidth[j]==75
           || mac_config[i].ul_bandwidth[j]==100))
        return 0;

    }

    if(&mac_config[i].dl_bandwidth !=NULL) {

      if(!(mac_config[i].dl_bandwidth[j]==6 || mac_config[i].dl_bandwidth[j]==15 || mac_config[i].dl_bandwidth[j]==25 || mac_config[i].dl_bandwidth[j]==50 || mac_config[i].dl_bandwidth[j]==75
           || mac_config[i].dl_bandwidth[j]==100))
        return 0;

    }

    if(&mac_config[i].min_ul_bandwidth !=NULL) {

      if(!(mac_config[i].min_ul_bandwidth[j]==6 || mac_config[i].min_ul_bandwidth[j]==15 || mac_config[i].min_ul_bandwidth[j]==25 || mac_config[i].min_ul_bandwidth[j]==50
           || mac_config[i].min_ul_bandwidth[j]==75 || mac_config[i].min_ul_bandwidth[j]==100))
        return 0;

    }

    if(&mac_config[i].min_dl_bandwidth !=NULL) {

      if(!(mac_config[i].min_dl_bandwidth[j]==6 || mac_config[i].min_dl_bandwidth[j]==15 || mac_config[i].min_dl_bandwidth[j]==25 || mac_config[i].min_dl_bandwidth[j]==50
           || mac_config[i].min_dl_bandwidth[j]==75 || mac_config[i].min_dl_bandwidth[j]==100))
        return 0;

    }

    if(&mac_config[i].ue_AggregatedMaximumBitrateDL !=NULL) {
      if(!(mac_config[i].ue_AggregatedMaximumBitrateDL>0 && mac_config[i].ue_AggregatedMaximumBitrateDL<10000000000))
        return 0;
    }

    if(&mac_config[i].ue_AggregatedMaximumBitrateUL !=NULL) {
      if(!(mac_config[i].ue_AggregatedMaximumBitrateUL>0 && mac_config[i].ue_AggregatedMaximumBitrateUL<10000000000))
        return 0;
    }

    if(&mac_config[i].cqiSchedInterval !=NULL) {
      if(!(mac_config[i].cqiSchedInterval==1 || mac_config[i].cqiSchedInterval==2 || mac_config[i].cqiSchedInterval==5 || mac_config[i].cqiSchedInterval==10 || mac_config[i].cqiSchedInterval==20
           || mac_config[i].cqiSchedInterval==32 || mac_config[i].cqiSchedInterval==40 || mac_config[i].cqiSchedInterval==64 || mac_config[i].cqiSchedInterval==80 || mac_config[i].cqiSchedInterval==128
           || mac_config[i].cqiSchedInterval==160))
        return 0;
    }

    if(&mac_config[i].mac_ContentionResolutionTimer !=NULL) {
      if(!(mac_config[i].mac_ContentionResolutionTimer==8 || mac_config[i].mac_ContentionResolutionTimer==16 || mac_config[i].mac_ContentionResolutionTimer==24
           || mac_config[i].mac_ContentionResolutionTimer==32 || mac_config[i].mac_ContentionResolutionTimer==40 || mac_config[i].mac_ContentionResolutionTimer==48
           || mac_config[i].mac_ContentionResolutionTimer==56 || mac_config[i].mac_ContentionResolutionTimer==64))
        return 0;

    }

    if(mac_config->max_allowed_rbs !=NULL) {

      if(mac_config[i].max_allowed_rbs[j]<=0)
        return 0;

    }

    if(mac_config[i].max_mcs !=NULL) {
      if(!(mac_config[i].max_mcs[j]>=0 && mac_config[i].max_mcs[j]<=27))
        return 0;
    }


  }

  if(event.key!=NULL && event.value!=NULL) {
    //printf("check one param");
    if(!strcmp((char *) event.key, "priority") && event.value!=NULL) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid ==-1) {
        for(i=0; i<MAX_NUM_LCID; i++) {
          for(j=0; j<MAX_NUM_LCID; j++) {

            if(&mac_config[i].priority[j]!=NULL) {
              if(mac_config[i].priority[j]<=0) {
                return 0;
              }

            }
          }
        }
      } else {
        i = event.ue;
        j = event.lcid;

        if(mac_config[i].priority[j]<=0) {
          return 0;
        }
      }
    } else if(!strcmp((char *) event.key, "DCI_aggregation_min") && event.value!=NULL) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;

      if(event.ue == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(mac_config[i].DCI_aggregation_min<=0) {
            return 0;
          }

        }
      } else {
        i=event.ue;

        if(mac_config[i].DCI_aggregation_min<=0) {
          return 0;
        }

      }
    } else if(!strcmp((char *) event.key, "DLSCH_dci_size_bits") && event.value!=NULL) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;



      if(event.ue == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(mac_config[i].DLSCH_dci_size_bits <=0) {
            return 0;
          }
        }
      } else {
        i = event.ue;

        if(mac_config[i].DLSCH_dci_size_bits <=0) {
          return 0;
        }
      }

    } else if(!strcmp((char *) event.key, "ul_bandwidth") && event.value!=NULL) {

      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {

          //mac_config[i] = *((Mac_config *) event.value + i);
          for(j=0; j<MAX_NUM_LCID; j++) {

            if(&mac_config[i].ul_bandwidth[j]!=NULL) {
              if(!(mac_config[i].ul_bandwidth[j]==6 || mac_config[i].ul_bandwidth[j]==15 || mac_config[i].ul_bandwidth[j]==25 || mac_config[i].ul_bandwidth[j]==50 || mac_config[i].ul_bandwidth[j]==75
                   || mac_config[i].ul_bandwidth[j]==100))
                return 0;

            }
          }

        }
      } else {
        i = event.ue;
        j = event.lcid;

        if(!(mac_config[i].ul_bandwidth[j]==6 || mac_config[i].ul_bandwidth[j]==15 || mac_config[i].ul_bandwidth[j]==25 || mac_config[i].ul_bandwidth[j]==50 || mac_config[i].ul_bandwidth[j]==75
             || mac_config[i].ul_bandwidth[j]==100))
          return 0;
      }

    } else if(!strcmp((char *) event.key, "min_ul_bandwidth") && event.value!=NULL) {

      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {

          //mac_config[i] = *((Mac_config *) event.value + i);
          for(j=0; j<MAX_NUM_LCID; j++) {

            if(&mac_config[i].min_ul_bandwidth[j]!=NULL) {
              if(!(mac_config[i].min_ul_bandwidth[j]==6 || mac_config[i].min_ul_bandwidth[j]==15 || mac_config[i].min_ul_bandwidth[j]==25 || mac_config[i].min_ul_bandwidth[j]==50
                   || mac_config[i].min_ul_bandwidth[j]==75 || mac_config[i].min_ul_bandwidth[j]==100))
                return 0;

            }
          }

        }
      } else {
        i = event.ue;
        j = event.lcid;

        if(!(mac_config[i].min_ul_bandwidth[j]==6 || mac_config[i].min_ul_bandwidth[j]==15 || mac_config[i].min_ul_bandwidth[j]==25 || mac_config[i].min_ul_bandwidth[j]==50
             || mac_config[i].min_ul_bandwidth[j]==75 || mac_config[i].min_ul_bandwidth[j]==100))
          return 0;
      }

    } else if(!strcmp((char *) event.key, "dl_bandwidth") && event.value!=NULL) {


      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(event.value + i !=NULL) {
            //mac_config[i] = *((Mac_config *) event.value + i);
            for(j=0; j<MAX_NUM_LCID; j++) {

              if(&mac_config[i].dl_bandwidth[j]!=NULL) {
                if(!(mac_config[i].dl_bandwidth[j]==6 || mac_config[i].dl_bandwidth[j]==15 || mac_config[i].dl_bandwidth[j]==25 || mac_config[i].dl_bandwidth[j]==50 || mac_config[i].dl_bandwidth[j]==75
                     || mac_config[i].dl_bandwidth[j]==100))
                  return 0;

              }
            }
          }
        }
      } else {
        i = event.ue;
        j = event.lcid;

        if(!(mac_config[i].dl_bandwidth[j]==6 || mac_config[i].dl_bandwidth[j]==15 || mac_config[i].dl_bandwidth[j]==25 || mac_config[i].dl_bandwidth[j]==50 || mac_config[i].dl_bandwidth[j]==75
             || mac_config[i].dl_bandwidth[j]==100))
          return 0;
      }
    } else if(!strcmp((char *) event.key, "min_dl_bandwidth") && event.value!=NULL) {


      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(event.value + i !=NULL) {
            //mac_config[i] = *((Mac_config *) event.value + i);
            for(j=0; j<MAX_NUM_LCID; j++) {

              if(&mac_config[i].min_dl_bandwidth[j]!=NULL) {
                if(!(mac_config[i].min_dl_bandwidth[j]==6 || mac_config[i].min_dl_bandwidth[j]==15 || mac_config[i].min_dl_bandwidth[j]==25 || mac_config[i].min_dl_bandwidth[j]==50
                     || mac_config[i].min_dl_bandwidth[j]==75 || mac_config[i].min_dl_bandwidth[j]==100))
                  return 0;

              }
            }
          }
        }
      } else {
        i = event.ue;
        j = event.lcid;

        if(!(mac_config[i].min_dl_bandwidth[j]==6 || mac_config[i].min_dl_bandwidth[j]==15 || mac_config[i].min_dl_bandwidth[j]==25 || mac_config[i].min_dl_bandwidth[j]==50
             || mac_config[i].min_dl_bandwidth[j]==75 || mac_config[i].dl_bandwidth[j]==100))
          return 0;
      }
    } else if(!strcmp((char *) event.key, "ue_AggregatedMaximumBitrateDL") && event.value!=NULL) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;

      LOG_I(EMU,"ue_AggregatedMaximumBitrateDL update \n");

      if(event.ue == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(!(mac_config[i].ue_AggregatedMaximumBitrateDL>0 && mac_config[i].ue_AggregatedMaximumBitrateDL<10000000000))
            return 0;
        }
      } else {
        i = event.ue;

        if(!(mac_config[i].ue_AggregatedMaximumBitrateDL>0 && mac_config[i].ue_AggregatedMaximumBitrateDL<10000000000))
          return 0;
      }

    } else if(!strcmp((char *) event.key, "ue_AggregatedMaximumBitrateUL") && event.value!=NULL) {

      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;



      if(event.ue == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(!(mac_config[i].ue_AggregatedMaximumBitrateUL>0 && mac_config[i].ue_AggregatedMaximumBitrateUL<10000000000))
            return 0;
        }
      } else {
        i = event.ue;

        if(!(mac_config[i].ue_AggregatedMaximumBitrateUL>0 && mac_config[i].ue_AggregatedMaximumBitrateUL<10000000000))
          return 0;
      }

    } else if(!strcmp((char *) event.key, "cqiSchedInterval") && event.value!=NULL) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;

      if(event.ue == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(!(mac_config[i].cqiSchedInterval==1 || mac_config[i].cqiSchedInterval==2 || mac_config[i].cqiSchedInterval==5 || mac_config[i].cqiSchedInterval==10 || mac_config[i].cqiSchedInterval==20
               || mac_config[i].cqiSchedInterval==32 || mac_config[i].cqiSchedInterval==40 || mac_config[i].cqiSchedInterval==64 || mac_config[i].cqiSchedInterval==80 || mac_config[i].cqiSchedInterval==128
               || mac_config[i].cqiSchedInterval==160))
            return 0;
        }
      } else {
        i = event.ue;

        if(!(mac_config[i].cqiSchedInterval==1 || mac_config[i].cqiSchedInterval==2 || mac_config[i].cqiSchedInterval==5 || mac_config[i].cqiSchedInterval==10 || mac_config[i].cqiSchedInterval==20
             || mac_config[i].cqiSchedInterval==32 || mac_config[i].cqiSchedInterval==40 || mac_config[i].cqiSchedInterval==64 || mac_config[i].cqiSchedInterval==80 || mac_config[i].cqiSchedInterval==128
             || mac_config[i].cqiSchedInterval==160))
          return 0;
      }
    } else if(!strcmp((char *) event.key, "mac_ContentionResolutionTimer") && event.value!=NULL) {
      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;

      if(event.ue == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(!(mac_config[i].mac_ContentionResolutionTimer==8 || mac_config[i].mac_ContentionResolutionTimer==16 || mac_config[i].mac_ContentionResolutionTimer==24
               || mac_config[i].mac_ContentionResolutionTimer==32 || mac_config[i].mac_ContentionResolutionTimer==40 || mac_config[i].mac_ContentionResolutionTimer==48
               || mac_config[i].mac_ContentionResolutionTimer==56 || mac_config[i].mac_ContentionResolutionTimer==64))
            return 0;
        }
      } else {
        i=event.ue;

        if(!(mac_config[i].mac_ContentionResolutionTimer==8 || mac_config[i].mac_ContentionResolutionTimer==16 || mac_config[i].mac_ContentionResolutionTimer==24
             || mac_config[i].mac_ContentionResolutionTimer==32 || mac_config[i].mac_ContentionResolutionTimer==40 || mac_config[i].mac_ContentionResolutionTimer==48
             || mac_config[i].mac_ContentionResolutionTimer==56 || mac_config[i].mac_ContentionResolutionTimer==64))
          return 0;
      }
    } else if(!strcmp((char *) event.key, "max_allowed_rbs") && event.value!=NULL && validate_mac(event)) {



      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          if(event.value + i !=NULL) {

            for(j=0; j<MAX_NUM_LCID; j++) {

              if(&mac_config[i].max_allowed_rbs[j]!=NULL) {
                if(mac_config[i].max_allowed_rbs[j]<=0)
                  return 0;
              }
            }
          }
        }
      } else {
        i = event.ue;
        j = event.lcid;

        if(mac_config[i].max_allowed_rbs[j]<=0)
          return 0;
      }

    } else if(!strcmp((char *) event.key, "max_mcs") && event.value!=NULL) {



      Mac_config* mac_config;// = malloc(sizeof(Mac_config)*16);
      mac_config = (Mac_config *) event.value;
      int j=0;

      if(event.ue == -1 && event.lcid == -1) {
        for(i=0; i<NUMBER_OF_UE_MAX; i++) {
          for(j=0; j<MAX_NUM_LCID; j++) {

            if(&mac_config[i].max_mcs[j]!=NULL) {
              if(!(mac_config[i].max_mcs[j]>=0 && mac_config[i].max_mcs[j]<=27))
                return 0;

            }
          }

        }
      } else {
        i = event.ue;
        j = event.lcid;

        if(!(mac_config[i].max_mcs[j]>=0 && mac_config[i].max_mcs[j]<=27))
          return 0;

      }
    }

  }

  return 1;


}

