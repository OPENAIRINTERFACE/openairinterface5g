/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
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
