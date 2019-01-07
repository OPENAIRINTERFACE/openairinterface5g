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
    C Header file <updatefw.h> for updatefw tool.

    K. Khalfallah, Aug, 2007
    kkhalfallah@free.fr
*/

#ifndef UPDATEFW_H
#define UPDATEFW_H

#include <stdio.h>
#include <errno.h>
#define _testsyscall(x, y, z)    {if (x==-1) {    \
                                    fprintf(stderr, "Error : %s() returned "z" (errno=%d)\n", y, x, errno);    \
                                    fprintf(stderr, "Warning : not leaving normally. Remove ipc ressources by hand !");    \
                                    fprintf(stderr, " (use shell commands ipcs & ipcrm)\n");    \
                                    exit(-1);    \
                                }}
//                                  else {    \
//                                    printf("Info : successfully called %s, returned "z"\n", y, x);}}

/* Intel is little-endian, sparc is big */
unsigned long ltmp;
#define invert4(x)      {ltmp=x; x=((ltmp & 0xff)<<24) | ((ltmp & 0xff00)<<8) | \
                         ((ltmp & 0xff0000)>>8) | ((ltmp & 0xff000000)>>24); }
unsigned short stmp;
#define invert2(x)    {stmp=x; x=((stmp & 0xff)<< 8) | ((stmp & 0xff00)>>8);}


/* static variables, global */
char* virt_addr;
unsigned int packet_ndx = 0;

#define FSEEK_ORIGIN        0
#define FSEEK_CURRENT        1
#define READ_FILE_MODE            "r"
/* Verbose levels */
#define VERBOSE_LEVEL_MAIN_STEPS        1
#define VERBOSE_LEVEL_ACCESS_FILES      2
#define VERBOSE_LEVEL_SECTION_DETAILS   3
#define VERBOSE_LEVEL_IOCTL             4


#endif /* UPDATEFW_H */
