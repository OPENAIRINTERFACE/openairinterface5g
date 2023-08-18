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

/**********************************************************************
*
* FILENAME    :  cic_filter_nr.h
*
* MODULE      :  synchronisation signal
*
* DESCRIPTION :  function related to nr synchronisation
*                It provides filters for sampling decimation
*
************************************************************************/

#ifndef CIC_FILTER_NR_H
#define CIC_FILTER_NR_H

#include "PHY/defs_nr_UE.h"
#include "PHY/types.h"

/************** DEFINE ********************************************/

#define M_DIFFERENTIAL_DELAY        (1)
#define FIR_RATE_CHANGE             (2)

/************** VARIABLES *****************************************/

#define FIR_TAPS_NUMBER                 (20)
#define FIR_FITER_SCALING_ACC           (14)

static const double sharpened_fir_taps[FIR_TAPS_NUMBER] = {
    /*
    cic filter and compensation FIR have been designed based on cic design tools provided at
    http://www.tsdconseil.fr/tutos/index.html
    -->cfir = cic_comp_design(4,4,1,30720000,2,1650000,20);
    R = 4.00, Fin = 30720000.00 Hz, Fout = 7680000.00 Hz.
    Attenuation for f > fout/2 : -14.82 dB.
    Attenuation at 1650000.00 Hz: -0.15 dB.
    Attenuation max. between 0 et 1650000.00 Hz: -0.15 dB.
    E.g. in linear scale : * 0.983
    Number of additionnal bits needed for implementation: 7.
    Fout = 7680000.00 Hz.
    index = 1026 / 4096.
    Correction Fint ?
    Filtre global :

    -->cfir
     cfir  =
    */
    -0.0059900, -0.0056191, 0.0107582, 0.0266043,  0.0130358,  -0.0331228, -0.0680440, -0.0278570, 0.1106335, 0.2897060,
    0.3607857,  0.2607354,  0.0983409, -0.0243749, -0.0583235, -0.0276023, 0.0104286,  0.0199533,  0.0071721, -0.0028096,
};

#define MAX_SAMPLEFILTER_TAP_NUM  (100)
#define FIR_SCALING_ACC           (15)

typedef struct {
  int32_t history[MAX_SAMPLEFILTER_TAP_NUM];
  int last_index;
  int filter_tap_number;
  int32_t *filter_taps;
} fir_filter_t;

#define SAMPLEFILTER_TAP_NUM       (59)

/*
 * This low pass filter was designed based on the tool at http://t-filter.engineerjs.com/
 * with below parameters
 * sampling frequency 30.72 MHz
 * from 	to 	          gain 	ripple/att.
 * 0 Hz     1.9 MHz        1     0.5 dB
 * 2 MHz     15 MHz        0     -20 dB
 */

static const int filter_taps[SAMPLEFILTER_TAP_NUM] = {
    -1572, 65,   -132, 197,  26,   393,  194,  536,  250,  514,  105,  279,  -234, -105, -646, -466, -924, -583, -856, -279,
    -321,  492,  639,  1601, 1812, 2766, 2872, 3650, 3503, 3976, 3503, 3650, 2872, 2766, 1812, 1601, 639,  492,  -321, -279,
    -856,  -583, -924, -466, -646, -105, -234, 279,  105,  514,  250,  536,  194,  393,  26,   197,  -132, 65,   -1572};

/************** FUNCTION ******************************************/

void integrator_stage(int32_t *input, int32_t *output, int length);

void comb_stage(int32_t *input, int32_t *output, int length, int differential_delay);

void rate_change_stage(int32_t *input, int32_t *output, int length, int rate_change);

void fir_filter(int32_t *input, int32_t *output, int length, int taps_fir_number, const int32_t *taps_fir, int scaling_shift);

void cic_decimator(int16_t *input_buffer, int16_t *output_buffer, int length, int rate_change, int number_cic_stage, int set_scaling_factor, int fir_rate_change);

void fir_decimator(int16_t *input_buffer, int16_t *output_buffer, int length, int rate_change, int scaling_factor);

#endif /* CIC_FILTER_NR_H */


