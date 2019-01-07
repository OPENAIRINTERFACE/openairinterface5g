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

/**
 * \file grid.h **/

#ifndef GRID_H_
#define GRID_H_

#include "omg.h"

int max_vertices_ongrid(omg_global_param omg_param);

int max_connecteddomains_ongrid(omg_global_param omg_param);


double vertice_xpos(int loc_num, omg_global_param omg_param);


double vertice_ypos(int loc_num, omg_global_param omg_param);


double area_minx(int block_num, omg_global_param omg_param);


double area_miny(int block_num, omg_global_param omg_param);

unsigned int next_block(int current_bn, omg_global_param omg_param);

unsigned int selected_blockn(int block_n,int type,int div);

#endif

