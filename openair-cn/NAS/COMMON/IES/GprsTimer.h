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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef GPRS_TIMER_H_
#define GPRS_TIMER_H_

#define GPRS_TIMER_MINIMUM_LENGTH 2
#define GPRS_TIMER_MAXIMUM_LENGTH 2

typedef struct GprsTimer_tag {
#define GPRS_TIMER_UNIT_2S  0b000 /* 2 seconds  */
#define GPRS_TIMER_UNIT_60S 0b001 /* 1 minute */
#define GPRS_TIMER_UNIT_360S  0b010 /* decihours  */
#define GPRS_TIMER_UNIT_0S  0b111 /* deactivated  */
  uint8_t  unit:3;
  uint8_t  timervalue:5;
} GprsTimer;

int encode_gprs_timer(GprsTimer *gprstimer, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_gprs_timer_xml(GprsTimer *gprstimer, uint8_t iei);

int decode_gprs_timer(GprsTimer *gprstimer, uint8_t iei, uint8_t *buffer, uint32_t len);

long gprs_timer_value(GprsTimer *gprstimer);

#endif /* GPRS TIMER_H_ */

