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
* Author and copyright: Laurent Thomas, open-cells.com
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

#include <common/utils/simple_executable.h>

#include "thread-pool.h"

#define SEP "\t"

uint64_t cpuCyclesMicroSec;

int main(int argc, char *argv[]) {
  if(argc != 2) {
    printf("Need one parameter: the trace Linux pipe (fifo)");
    exit(1);
  }

  mkfifo(argv[1],0666);
  int fd=open(argv[1], O_RDONLY);

  if ( fd == -1 ) {
    perror("open read mode trace file:");
    exit(1);
  }

  uint64_t deb=rdtsc_oai();
  usleep(100000);
  cpuCyclesMicroSec=(rdtsc_oai()-deb)/100000;
  printf("Cycles per µs: %lu\n",cpuCyclesMicroSec);
  printf("Key" SEP "delay to process" SEP "processing time" SEP "delay to be read answer\n");
  notifiedFIFO_elt_t doneRequest;

  while ( 1 ) {
    if ( read(fd,&doneRequest, sizeof(doneRequest)) ==  sizeof(doneRequest)) {
      printf("%lu" SEP "%lu" SEP "%lu" SEP "%lu" "\n",
             doneRequest.key,
             (doneRequest.startProcessingTime-doneRequest.creationTime)/cpuCyclesMicroSec,
             (doneRequest.endProcessingTime-doneRequest.startProcessingTime)/cpuCyclesMicroSec,
             (doneRequest.returnTime-doneRequest.endProcessingTime)/cpuCyclesMicroSec
            );
    } else {
      printf("no measurements\n");
      sleep(1);
    }
  }
}
