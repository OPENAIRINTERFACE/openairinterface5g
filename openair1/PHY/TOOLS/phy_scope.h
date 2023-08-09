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
 * Author and copyright: Laurent Thomas, open-cells.com
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
#include "phy_scope_interface.h"

#ifndef phy_scope_h_
#define phy_scope_h_

#define TPUT_WINDOW_LENGTH 100
#define ScaleZone 4
#define localBuff(NaMe, SiZe) \
  float NaMe[SiZe];           \
  memset(NaMe, 0, sizeof(NaMe));

/* scope chart window id's, used to select which frontend chart to use for displaying data */
#define SCOPEMSG_DATAID_IQ 1
#define SCOPEMSG_DATAID_LLR 2
#define SCOPEMSG_DATAID_WF 3
#define SCOPEMSG_DATAID_TRESP 4

typedef c16_t scopeSample_t;
#define SquaredNorm(VaR) ((VaR).r * (VaR).r + (VaR).i * (VaR).i)

typedef struct OAIgraph {
  FL_OBJECT *graph;
  FL_OBJECT *text;
  float maxX;
  float maxY;
  float minX;
  float minY;
  int x;
  int y;
  int w;
  int h;
  int waterFallh;
  double *waterFallAvg;
  bool initDone;
  int iteration;
  void (*gNBfunct)(struct OAIgraph *graph, scopeData_t *p, int UE_id);
  void (*nrUEfunct)(scopeGraphData_t **data, struct OAIgraph *graph, PHY_VARS_NR_UE *phy_vars_ue, int eNB_id, int UE_id);
  bool enabled;
  char chartid;
  char datasetid;
} OAIgraph_t;

/* Forms and Objects */
typedef struct {
  FL_FORM *phy_scope;
  OAIgraph_t graph[20];
  FL_OBJECT *button_0;
} OAI_phy_scope_t;

typedef struct {
  FL_FORM *stats_form;
  void *vdata;
  char *cdata;
  long ldata;
  FL_OBJECT *stats_text;
  FL_OBJECT *stats_button;
} FD_stats_form;
#ifdef WEBSRVSCOPE
extern OAI_phy_scope_t *create_phy_scope_gnb(void);
extern OAI_phy_scope_t *create_phy_scope_nrue(int ueid);
extern void phy_scope_gNB(OAI_phy_scope_t *form, scopeData_t *p, int nb_UE);
extern void phy_scope_nrUE(scopeGraphData_t **UEliveData, OAI_phy_scope_t *form, PHY_VARS_NR_UE *phy_vars_ue, int eNB_id, int UE_id);
extern void nrUEinitScope(PHY_VARS_NR_UE *ue);
extern void gNBinitScope(scopeParms_t *p);
typedef int16_t chart_data_t;
#endif
#endif
