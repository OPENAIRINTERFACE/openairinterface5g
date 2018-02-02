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

/*****************************************************************************
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    as_data.c

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem Access-Stratum data

Author    Frederic Maurel

Description Defines constants and functions used by the AS simulator
    process.

*****************************************************************************/

#include "as_data.h"

#include "as_message.h"

#include <stdio.h>  // snprintf
#include <time.h> // clock_gettime

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *        Time stamp generator
 * -----------------------------------------------------------------------------
 */
const char* getTime(void)
{
#define TIME_BUFFER_SIZE  16
  static char time_buffer[TIME_BUFFER_SIZE];
  struct timespec ts;

  clock_gettime (CLOCK_REALTIME, &ts);
  snprintf(time_buffer, TIME_BUFFER_SIZE, "%.6ld:%.6ld",
           ts.tv_sec % 3600, ts.tv_nsec / 1000);
  return (time_buffer);
}

/*
 * -----------------------------------------------------------------------------
 *          Return RRC establishment cause
 * -----------------------------------------------------------------------------
 */
const char* rrcCause(char cause)
{
  if (cause == AS_CAUSE_EMERGENCY) {
    return "EMERGENCY";
  } else if (cause == AS_CAUSE_HIGH_PRIO) {
    return "HIGH_PRIORITY";
  } else if (cause == AS_CAUSE_MT_ACCESS) {
    return "MT_ACCESS";
  } else if (cause == AS_CAUSE_MO_SIGNAL) {
    return "MO_SIGNALLING";
  } else if (cause == AS_CAUSE_MO_DATA) {
    return "MO_DATA";
  } else if (cause == AS_CAUSE_V1020) {
    return "V1020";
  } else {
    return "Unknown";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return RRC call type
 * -----------------------------------------------------------------------------
 */
const char* rrcType(char type)
{
  if (type == AS_TYPE_ORIGINATING_SIGNAL) {
    return "ORIGINATING_SIGNAL";
  } else if (type == AS_TYPE_EMERGENCY_CALLS) {
    return "EMERGENCY_CALLS";
  } else if (type == AS_TYPE_ORIGINATING_CALLS) {
    return "ORIGINATING_CALLS";
  } else if (type == AS_TYPE_TERMINATING_CALLS) {
    return "TERMINATING_CALLS";
  } else if (type == AS_TYPE_MO_CS_FALLBACK) {
    return "MO_CS_FALLBACK";
  } else {
    return "Unknown";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return Error Code
 * -----------------------------------------------------------------------------
 */
const char* rrcErrCode(char code)
{
  if (code == AS_SUCCESS) {
    return "SUCCESS";
  } else if (code == AS_TERMINATED_NAS) {
    return "TERMINATED_NAS";
  } else if (code == AS_TERMINATED_AS) {
    return "TERMINATED_AS";
  } else if (code == AS_FAILURE) {
    return "FAILURE";
  } else {
    return "Unknown";
  }
}

/*
 * -----------------------------------------------------------------------------
 *        Return NAS release cause
 * -----------------------------------------------------------------------------
 */
const char* rrcReleaseCause(char cause)
{
  if (cause == AS_AUTHENTICATION_FAILURE) {
    return "AUTHENTICATION FAILURE";
  } else if (cause == AS_DETACH) {
    return "DETACH";
  } else {
    return "Unknown";
  }
}

