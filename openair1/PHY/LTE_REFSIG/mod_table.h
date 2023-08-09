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

#ifndef __PHY_LTE_REFSIG_MOD_TABLE__H__
#define __PHY_LTE_REFSIG_MOD_TABLE__H__

#define MOD_TABLE_SIZE_SHORT 304
#define MOD_TABLE_QPSK_OFFSET 1
#define MOD_TABLE_16QAM_OFFSET 5
#define MOD_TABLE_64QAM_OFFSET 21
#define MOD_TABLE_PSS_OFFSET 85
short mod_table[MOD_TABLE_SIZE_SHORT] = {0,0,768,768,768,-768,-768,768,-768,-768,343,343,343,1030,1030,343,1030,1030,343,-343,343,-1030,1030,-343,1030,-1030,-343,343,-343,1030,-1030,343,-1030,1030,-343,-343,-343,-1030,-1030,-343,-1030,-1030,503,503,503,168,168,503,168,168,503,838,503,1173,168,838,168,1173,838,503,838,168,1173,503,1173,168,838,838,838,1173,1173,838,1173,1173,503,-503,503,-168,168,-503,168,-168,503,-838,503,-1173,168,-838,168,-1173,838,-503,838,-168,1173,-503,1173,-168,838,-838,838,-1173,1173,-838,1173,-1173,-503,503,-503,168,-168,503,-168,168,-503,838,-503,1173,-168,838,-168,1173,-838,503,-838,168,-1173,503,-1173,168,-838,838,-838,1173,-1173,838,-1173,1173,-503,-503,-503,-168,-168,-503,-168,-168,-503,-838,-503,-1173,-168,-838,-168,-1173,-838,-503,-838,-168,-1173,-503,-1173,-168,-838,-838,-838,-1173,-1173,-838,-1173,-1173,1086,0,1081,-108,1065,-215,1038,-320,1001,-422,954,-519,897,-612,832,-698,758,-777,677,-849,589,-912,495,-966,397,-1011,294,-1046,189,-1070,81,-1083,-27,-1086,-135,-1078,-242,-1059,-346,-1030,-447,-990,-543,-941,-634,-882,-719,-814,-796,-739,-866,-656,-927,-566,-979,-471,-1021,-371,-1053,-268,-1074,-162,-1085,-54,-1085,54,-1074,162,-1053,268,-1021,371,-979,471,-927,566,-866,656,-796,739,-719,814,-634,882,-543,941,-447,990,-346,1030,-242,1059,-135,1078,-27,1086,81,1083,189,1070,294,1045,397,1011,495,966,589,912,677,849,758,777,832,698,897,612,954,519,1001,422,1038,320,1065,215,1081,108,16384,0,8192,0,4096,0,2048,0};

#endif
