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

/*! \file openair1/PHY/TOOLS/phy_scope_interface.c
 * \brief soft scope API interface implementation
 * \author Nokia BellLabs France, francois Taburet
 * \date 2019
 * \version 0.1
 * \company Nokia BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <stdio.h>
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#include "phy_scope_interface.h"

#define SOFTSCOPE_ENDFUNC_IDX 0

static  loader_shlibfunc_t scope_fdesc[]= {{"end_forms",NULL}};

int copyDataMutexInit(scopeData_t *p)
{
  return pthread_mutex_init(&p->copyDataMutex, NULL);
}

int load_softscope(char *exectype, void *initarg) {
  char libname[64];
  sprintf(libname,"%.10sscope",exectype);
  return load_module_shlib(libname,scope_fdesc,1,initarg);
}

int end_forms(void) {
  if (scope_fdesc[SOFTSCOPE_ENDFUNC_IDX].fptr) {
    scope_fdesc[SOFTSCOPE_ENDFUNC_IDX].fptr();
    return 0;
  }

  return -1;
}

void copyData(void *scopeData, enum scopeDataType type, void *dataIn, int elementSz, int colSz, int lineSz, int offset)
{
  scopeData_t *tmp = (scopeData_t *)scopeData;

  if (tmp) {
    // Begin of critical zone between UE Rx threads that might copy new data at the same time:
    pthread_mutex_lock(&tmp->copyDataMutex);
    scopeGraphData_t *oldData = ((scopeGraphData_t **)tmp->liveData)[type];
    tmp->copyDataBufsIdx[type] = (tmp->copyDataBufsIdx[type] + 1) % COPIES_MEM;
    int newCopyDataIdx = tmp->copyDataBufsIdx[type];
    pthread_mutex_unlock(&tmp->copyDataMutex);
    // End of critical zone between UE Rx threads
    int oldDataSz = oldData ? oldData->dataSize : 0;
    int newSz = max(elementSz * colSz * (lineSz + offset), oldDataSz);
    // New data will be copied in a different buffer than the live one
    scopeGraphData_t *newData = tmp->copyDataBufs[type][newCopyDataIdx];
    if (newData == NULL || newData->dataSize < newSz) {
      scopeGraphData_t *ptr = (scopeGraphData_t *)realloc(newData, sizeof(scopeGraphData_t) + newSz);
      if (!ptr) {
        LOG_E(PHY, "can't realloc\n");
        return;
      } else {
        tmp->copyDataBufs[type][newCopyDataIdx] = ptr;
        if (!newData) // we have a new malloc
          *ptr = (scopeGraphData_t){0};
        newData = ptr;
        newData->dataSize = newSz;
      }
    }
    if (offset && oldDataSz) // we copy the previous buffer because we have as input only a part of
      memcpy(newData + 1, oldData + 1, oldDataSz);

    newData->elementSz = elementSz;
    newData->colSz = colSz;
    newData->lineSz = lineSz + offset;
    memcpy(((void *)(newData + 1)) + elementSz * colSz * offset, dataIn, elementSz * colSz * lineSz);

    // The new data just copied becomes live now
    ((scopeGraphData_t **)tmp->liveData)[type] = newData;
  }
}
