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

/*
 * logs.h
 *
 *  Created on: Nov 22, 2013
 *      Author: Laurent Winckel
 */

#ifndef LOGS_H_
#define LOGS_H_

/* Added definition of the g_info log function to complete the set of log functions from "gmessages.h" */

#include <glib/gmessages.h>

#ifdef G_HAVE_ISO_VARARGS

#define g_info(...)     g_log (G_LOG_DOMAIN,        \
                               G_LOG_LEVEL_INFO,    \
                               __VA_ARGS__)

#else

static void
g_info (const gchar *format,
        ...)
{
  va_list args;
  va_start (args, format);
  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format, args);
  va_end (args);
}

#endif

#endif /* LOGS_H_ */
