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
uint16_t Mcsindextable1[29][2] = {{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},{4,340},{4,378},{4,434},{4,490},{4,553},{4,616},
		{4,658},{6,438},{6,466},{6,517},{6,567},{6,616},{6,666},{6,719},{6,772},{6,822},{6,873}, {6,910}, {6,948}};
//Table 5.1.2.2-2
uint16_t Tbstable_nr[INDEX_MAX_TBS_TABLE] = {24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,208,224,240,256,272,288,304,320,336,352,368,384,408,432,456,480,504,528,552,576,608,640,672,704,736,768,808,848,888,928,984,1032,1064,1128,1160,1192,1224,1256,1288,1320,1352,1416,1480,1544,1608,1672,1736,1800,1864,1928,2024,2088,2152,2216,2280,2408,2472,2536,2600,2664,2728,2792,2856,2976,3104,3240,3368,3496,3624,3752,3824};

uint32_t nr_compute_tbs(uint8_t mcs,
						uint16_t nb_rb,
						uint16_t nb_symb_sch,
						uint8_t nb_re_dmrs,
						uint16_t length_dmrs,
						uint8_t Nl)
{
	uint16_t nbp_re, nb_re, nb_dmrs_prb, nb_rb_oh,Qm,R;
	uint32_t nr_tbs=0;
	double Ninfo,Np_info,n,C;

	nb_rb_oh = 0; //set to 0 if not configured by higher layer
	Qm = Mcsindextable1[mcs][0];
	R  = Mcsindextable1[mcs][1];
	nb_dmrs_prb = nb_re_dmrs*length_dmrs;

	nbp_re = 12 * nb_symb_sch - nb_dmrs_prb - nb_rb_oh;

	nb_re = min(156, nbp_re) * nb_rb;

    // Intermediate number of information bits
    Ninfo = (double)((nb_re * R * Qm * Nl)/1024);

    //printf("Ninfo %lf nbp_re %d nb_re %d mcs %d Qm %d, R %d\n", Ninfo, nbp_re, nb_re,mcs, Qm, R);

    if (Ninfo <=3824) {

    	n = max(3, floor(log2(Ninfo)) - 6);
        Np_info = max(24, pow(2,n) * floor(Ninfo/pow(2,n)));

        for (int i=0; i<INDEX_MAX_TBS_TABLE; i++) {
        	if ((double)Tbstable_nr[i] >= Np_info){
        		nr_tbs = Tbstable_nr[i];
        		break;
        	}
        }
    }
    else {
    	n = floor(log2(Ninfo-24)) - 5;
        Np_info = max(3840, pow(2,n) * round((Ninfo - 24)/pow(2,n)));

        if (R <= 256) { //1/4
            C = ceil( (Np_info + 24)/3816 );
            nr_tbs = (uint32_t)(8 * C * ceil( (Np_info + 24)/(8*C) ) - 24);
        }
        else {
            if (Np_info > 8424){
                C = ceil( (Np_info + 24)/8424 );
                nr_tbs = (uint32_t)(8 * C * ceil( (Np_info + 24)/(8*C) ) - 24);
            }
            else {
            	nr_tbs = (uint32_t)(8 * ceil( (Np_info + 24)/8 ) - 24);
            	//printf("n %lf Np_info %f pow %f ceil %f \n",n, Np_info,pow(2,6),ceil( (Np_info + 24)/8 ));
            }

        }

    }

	return nr_tbs;
}
