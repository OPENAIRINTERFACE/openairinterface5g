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

/* file: nr_compute_tbs.c
   purpose: Compute NR TBS
   author: Hongzhi WANG (TCL)
*/
#include "PHY/defs_nr_UE.h"
//#include "SCHED/extern.h"

#define INDEX_MAX_TBS_TABLE (93)
//Table 5.1.3.1-1
uint16_t Mcsindextable1[29][3] = {{2,120,0.2344},
           	   	   	   	   	   	 {2,157,0.3066},
								 {2,193,0.3770},
								 {2,251,0.4902},
								 {2,308,0.6016},
								 {2,379,0.7402},
								 {2,449,0.8770},
								 {2,526,1.0273},
								 {2,602,1.1758},
								 {2,679,1.3262},
								 {4,340,1.3281},
								 {4,378,1.4766},
								 {4,434,1.6953},
								 {4,490,1.9141},
								 {4,553,2.1602},
								 {4,616,2.4063},
								 {4,658,2.5703},
								 {6,438,2.5664},
								 {6,466,2.7305},
								 {6,517,3.0293},
								 {6,567,3.3223},
								 {6,616,3.6094},
								 {6,666,3.9023},
								 {6,719,4.2129},
								 {6,772,4.5234},
								 {6,822,4.8164},
								 {6,873,5.1152},
								 {6,910,5.3320},
								 {6,948,5.5547}};
//Table 5.1.2.2-2
uint16_t Tbstable_nr[INDEX_MAX_TBS_TABLE] = {24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,208,224,240,256,272,288,304,320,336,352,368,384,408,432,456,480,504,528,552,576,608,640,672,704,736,768,808,848,888,928,984,1032,1064,1128,1160,1192,1224,1256,1288,1320,1352,1416,1480,1544,1608,1672,1736,1800,1864,1928,2024,2088,2152,2216,2280,2408,2472,2536,2600,2664,2728,2792,2856,2976,3104,3240,3368,3496,3624,3752,3824};

uint32_t nr_compute_tbs(uint8_t mcs,
						uint16_t nb_rb,
						uint16_t nb_symb_sch,
						uint8_t nb_re_dmrs,
						uint16_t length_dmrs,
						uint8_t Nl)
{
	uint16_t nbp_re, nb_re, nb_dmrs_prb, nb_rb_oh, Ninfo,Np_info,n,Qm,R,C;
	uint32_t nr_tbs;

	nb_rb_oh = 0; //set to 0 if not configured by higher layer
	Qm = Mcsindextable1[mcs][0];
	R  = Mcsindextable1[mcs][1];
	nb_dmrs_prb = nb_re_dmrs*length_dmrs;

	nbp_re = 12 * nb_symb_sch - nb_dmrs_prb - nb_rb_oh;

	nb_re = min(156, nbp_re) * nb_rb;

    // Intermediate number of information bits
    Ninfo = (nb_re * R * Qm * Nl)/1024;

    //printf("Ninfo %d nbp_re %d nb_re %d Qm %d, R %d\n", Ninfo, nbp_re, nb_re, Qm, R);

    if (Ninfo <=3824) {

    	n = max(3, floor(log2(Ninfo)) - 6);
        Np_info = max(24, pow(n,2) * floor(Ninfo/pow(n,2)));

        for (int i=0; i<INDEX_MAX_TBS_TABLE; i++) {
        	if (Tbstable_nr[i] >= Np_info){
        		nr_tbs = Tbstable_nr[i];
        		break;
        	}
        }
    }
    else {
    	n = floor(log2(Ninfo-24)) - 5;
        Np_info = max(3840, pow(n,2) * round((Ninfo - 24)/pow(n,2)));

        if (R <= 1/4) {
            C = ceil( (Np_info + 24)/3816 );
            nr_tbs = 8 * C * ceil( (Np_info + 24)/(8*C) ) - 24;
        }
        else {
            if (Np_info > 8424){
                C = ceil( (Np_info + 24)/8424 );
                nr_tbs = 8 * C * ceil( (Np_info + 24)/(8*C) ) - 24;
            }
            else
            	nr_tbs = 8 * ceil( (Np_info + 24)/8 ) - 24;

        }

    }

	return nr_tbs;
}
