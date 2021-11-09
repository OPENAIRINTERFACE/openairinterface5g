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

/* \file rrc_list.h
 * \brief linked list implementation for ToAddModList mechanism in RRC layer
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef _RRC_LIST_H_
#define _RRC_LIST_H_


#define RRC_LIST_TYPE(T, N)     \
    struct {                    \
        T *entries[N];          \
        int next[N];            \
        int prev[N];            \
        int start;              \
        int count;              \
    }

//  initial function for the certain list, storage number of entry, initial pointer and corresponding links
#define RRC_LIST_INIT(list, c)                      \
    do {                                            \
        int iterator;                               \
        (list).count = (c);                         \
        for(iterator=0; iterator<c; ++iterator){    \
            (list).entries[iterator] = NULL;        \
            (list).next[iterator] = -1;             \
            (list).prev[iterator] = -1;             \
            (list).start = -1;                      \
        }                                           \
    }while(0)


//  check the entry by id first then update or create new entry.
#define RRC_LIST_MOD_ADD(list, new, id_name)                                        \
    do {                                                                            \
        int iterator;                                                               \
        for(iterator=(list).start; iterator!=-1; iterator=(list).next[iterator]){   \
            if((new)->id_name == (list).entries[iterator]->id_name){                \
                (list).entries[iterator] = (new);                                   \
                break;                                                              \
            }                                                                       \
        }                                                                           \
        if(iterator==-1){                                                           \
            for(iterator=0; iterator<(list).count; ++iterator){                     \
                if((list).entries[iterator] == NULL){                               \
                    (list).next[iterator] = (list).start;                           \
                    (list).prev[iterator] = -1;                                     \
                    if((list).start != -1){                                         \
                        (list).prev[list.start] = iterator;                         \
                    }                                                               \
                    (list).start = iterator;                                        \
                    (list).entries[iterator] = (new);                               \
                    break;                                                          \
                }                                                                   \
            }                                                                       \
        }                                                                           \
    }while(0)

//  search entries by id, unlink from the list and output free pointer for upper function to release memory
#define RRC_LIST_MOD_REL(list, id_name, id, free)                                   \
    do{                                                                             \
        int iterator;                                                               \
        for(iterator=(list).start; iterator!=-1; iterator=(list).next[iterator]){   \
            if(id == (list).entries[iterator]->id_name){                            \
                if((list).prev[iterator] == -1){                                    \
                    (list).start = (list).next[iterator];                           \
                }else{                                                              \
                    (list).next[(list).prev[iterator]] = (list).next[iterator];     \
                }                                                                   \
                if((list).next[iterator] != -1){                                    \
                    (list).prev[(list).next[iterator]] = (list).prev[iterator];     \
                }                                                                   \
                (free) = (list).entries[iterator];                                  \
                (list).entries[iterator] = NULL;                                    \
                break;                                                              \
            }                                                                       \
        }                                                                           \
    }while(0)


#define RRC_LIST_FOREACH(list, i)       \
        for((i)=(list).start; (i) != -1; (i)=(list).next[i])

#define RRC_LIST_ENTRY(list, i)         \
        list.entries[i]

#endif
