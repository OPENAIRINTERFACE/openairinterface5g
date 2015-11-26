/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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
