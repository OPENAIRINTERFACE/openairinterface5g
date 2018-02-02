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

/*! \file otg_models.h
* \brief Data structure and functions for OTG
* \author M. Laner and navid nikaein
* \date 2013
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OTG_MODELS_H__
# define __OTG_MODELS_H__



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>




/*function declarations*/


/*tarma*/
double tarmaCalculateSample( double inputSamples[], tarmaProcess_t *proc);
void tarmaUpdateInputSample (tarmaStream_t *stream);
tarmaStream_t *tarmaInitStream(tarmaStream_t *stream);
void tarmaSetupOpenarenaDownlink(tarmaStream_t *stream);
void tarmaPrintProc(tarmaProcess_t *proc);
void tarmaPrintStreamInit(tarmaStream_t *stream);

/*tarma video*/
void tarmaPrintVideoInit(tarmaVideo_t *video);
tarmaVideo_t *tarmaInitVideo(tarmaVideo_t *video);
double tarmaCalculateVideoSample(tarmaVideo_t *video);
void tarmaSetupVideoGop12(tarmaVideo_t *video, double compression);

/*background*/
backgroundStream_t *backgroundStreamInit(backgroundStream_t *stream, double lambda_n);
void backgroundUpdateStream(backgroundStream_t *stream, int ctime);
double backgroundCalculateSize(backgroundStream_t *stream, int ctime, int idt);
void backgroundPrintStream(backgroundStream_t *stream);

#endif
