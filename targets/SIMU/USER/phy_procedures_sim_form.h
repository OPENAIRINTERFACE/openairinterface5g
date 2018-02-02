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

/** Header file generated with fdesign on Tue Jul 27 16:31:42 2010.**/

#ifndef FD_phy_procedures_sim_h_
#define FD_phy_procedures_sim_h_

/** Callbacks, globals and object handlers **/


/**** Forms and Objects ****/
typedef struct {
  FL_FORM *phy_procedures_sim;
  void *vdata;
  char *cdata;
  long  ldata;
  FL_OBJECT *pusch_constellation;
  FL_OBJECT *pdsch_constellation;
  FL_OBJECT *ch00;
} FD_phy_procedures_sim;

extern FD_phy_procedures_sim * create_form_phy_procedures_sim(void);

#endif /* FD_phy_procedures_sim_h_ */
