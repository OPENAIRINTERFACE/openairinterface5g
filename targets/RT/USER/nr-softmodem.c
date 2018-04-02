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

#include "nr-softmodem.h"
#include "PHY/types.h"
#include "PHY/defs_NR.h"

//Temporary main function

void exit_fun(const char* s)
{

  int ru_id;

  if (s != NULL) {
    printf("%s %s() Exiting OAI softmodem: %s\n",__FILE__, __FUNCTION__, s);
  }

  /*oai_exit = 1;


    if (RC.ru == NULL)
        exit(-1); // likely init not completed, prevent crash or hang, exit now...
    for (ru_id=0; ru_id<RC.nb_RU;ru_id++) {
      if (RC.ru[ru_id] && RC.ru[ru_id]->rfdevice.trx_end_func)
	RC.ru[ru_id]->rfdevice.trx_end_func(&RC.ru[ru_id]->rfdevice);
      if (RC.ru[ru_id] && RC.ru[ru_id]->ifdevice.trx_end_func)
	RC.ru[ru_id]->ifdevice.trx_end_func(&RC.ru[ru_id]->ifdevice);  
    }*/
}

int main( int argc, char **argv )
{
  nfapi_config_request_t config;
  NR_DL_FRAME_PARMS* frame_parms = malloc(sizeof(NR_DL_FRAME_PARMS));
  int16_t amp;
  //malloc to move
  int16_t** txdataF = (int16_t **)malloc(2048*2*14*2*2* sizeof(int16_t));
  int16_t* d_pss = malloc(NR_PSS_LENGTH * sizeof(int16_t));
  int16_t *d_sss = malloc(NR_SSS_LENGTH * sizeof(int16_t));

  //logInit();

  phy_init_nr_gNB(&config);
  nr_init_frame_parms(config, frame_parms);
  nr_dump_frame_parms(frame_parms);

  amp = 32767; //1_Q_15
  //nr_generate_pss(d_pss, txdataF, amp, 0, 0, config, frame_parms);
  nr_generate_sss(d_sss, txdataF, amp, 0, 0, config, frame_parms);

  free(txdataF);
  free(d_pss);
  free(d_sss);

  return 0;
}
