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
/*****************************************************************************
Source      userDef.h

Version     0.1

Date        2012/09/21

Product     NAS stack

Subsystem   include

Author      Frederic Maurel

Description Contains user's global definitions

*****************************************************************************/
#ifndef __USERDEF_H__
#define __USERDEF_H__

#include <stdint.h>

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * The name of the file used as non-volatile memory device to store
 * UE data parameters
 */
#define USER_NVRAM_FILENAME ".ue.nvram"

/*
 * The name of the environment variable which defines the directory
 * where the UE data file is located
 */
#define USER_NVRAM_DIRNAME  "NVRAM_DIR"

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * ------------------------------
 * Structure of the UE parameters
 * ------------------------------
 */
typedef struct {
  /* International Mobile Equipment Identity  */
#define USER_IMEI_SIZE          15
  char IMEI[USER_IMEI_SIZE+1];
  /* Manufacturer identifier          */
#define USER_MANUFACTURER_SIZE      16
  char manufacturer[USER_MANUFACTURER_SIZE+1];
  /* Model identifier             */
#define USER_MODEL_SIZE         16
  char model[USER_MODEL_SIZE+1];
  /* SIM Personal Identification Number   */
#define USER_PIN_SIZE           4
  char PIN[USER_PIN_SIZE+1];
} user_nvdata_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* __USERDEF_H__*/
