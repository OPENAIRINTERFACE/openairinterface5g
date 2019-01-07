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

/*! \file mobility_parser.c
* \brief A parser for trace-based mobility information (parsed from a file)
* \author  S. Gashaw, N. Nikaein,  J. Harri
* \date 2014
* \version 0.1
* \company EURECOM
* \email:
* \note
* \warning
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mobility_parser.h"
#include "omg.h"
#include <math.h>
#include "assertions.h"

extern hash_table_t **table;
extern node_info **list_head;

//read the mobility file and generates a hashtable of linked list
void
parse_data (char *trace_file, int node_type)
{
  FILE *fp;
  char *pch, *p;
  char str[128];
  int fmt, id = 0, gid;
  node_container *value;

  //if(table==NULL)
  create_new_table (node_type);

  if(list_head ==NULL)
    list_head = (node_info **) calloc (MAX_NUM_NODE_TYPES, sizeof (node_info*));

  if (list_head == NULL ) {
    LOG_E (OMG, "-------node list table creation failed--------\n");
    exit (-1);
  }


  if ((fp = fopen (trace_file, "r")) == NULL) {
    LOG_E (OMG, "[OMG]:Cannot open file %s\n", trace_file);
    exit (1);
  }


  while (!feof (fp)) {
    if (fgets (str, 126, fp)) {
      fmt = 0;
      p = str;

      while (*p == ' ' || *p == '\t' || *p == '\r')
        p++;    // skip whitespaces

      if (*p != '\n') { //escape empty line
        pch = strtok (p, " ");  // the separator between the items in the list is a space
        node_data *node = (node_data *) calloc (1, sizeof (node_data));
        node->type=-1;

        while (pch != NULL) {

          switch (fmt) {
          case 0:
            if (atof (pch) < 0)
              LOG_E (OMG, "error: negative time input \n");

            node->time = fabs (atof (pch));
            LOG_D (OMG, "%.2f \n",node->time);
            break;

          case 1:
            node->vid = atoi (pch);
            LOG_D (OMG, "%d \n",node->vid);
            break;

          case 2:
            node->x_pos = atof (pch);
            LOG_D (OMG, "%.2f \n",node->x_pos);
            break;

          case 3:
            node->y_pos = atof (pch);
            LOG_D (OMG, "%.2f \n",node->y_pos);
            break;

          case 4:
            if (atof (pch) < 0)
              LOG_D (OMG, "error: negative speed input");

            node->speed = fabs (atof (pch));
            LOG_D (OMG, "speed %.2f \n",node->speed);
            break;

            /*case 5:
               node->type = atof (pch);
               break;*/
          default:
            LOG_E (OMG,
                   "[Error in trance file]:incorrect data format \n");
            break;
          }

          fmt += 1;
          pch = strtok (NULL, " ");
        }

        node->next = NULL;
        node->visit = 0;

        // look for node in node info
        gid = find_node_info (node->vid, node_type);

        if (gid == -1) {
          node->gid = id;
          add_node_info (node->vid, node->gid, node_type);
          //store node data in the table
          hash_table_add (table[node_type], node, NULL);
          id++;
        } else {
          node->gid = gid;
          value = hash_table_lookup (table[node_type], node->gid);
          hash_table_add (table[node_type], node, value);
        }



      }
    }
  }

  fclose (fp);

}

//search for node given id if exist

int
find_node_info (int vid, int node_type)
{
  node_info *search;

  if (list_head[node_type] != NULL) {
    search = list_head[node_type];

    while (search != NULL) {
      if (search->vid == vid)
        return search->g_id;

      search = search->next;
    }
  }

  return -1;
}

//builds linked list with each node holding vehicle id read from file & given id
void
add_node_info (int nid, int n_gid, int node_type)
{
  node_info *new = (node_info *) malloc (sizeof (node_info));

  if (new != NULL) {
    new->vid = nid;
    new->g_id = n_gid;
    new->next = NULL;

    if (list_head[node_type] == NULL)
      list_head[node_type] = new;
    else {
      node_info *temp, *save;
      temp = list_head[node_type];

      while (temp != NULL) {
        save = temp;
        temp = temp->next;
      }

      save->next = new;
    }
  } else {
    LOG_E (OMG, "node info  list creation failed\n");
    exit (-1);
  }
}



int
get_number_of_nodes (int node_type)
{
  return table[node_type]->key_count;
}

node_data *
get_next_data (hash_table_t * table, int vid, int flag)
{
  node_container *block = hash_table_lookup (table, vid);
  node_data *links = NULL, *save = NULL;

  if (block != NULL)
    links = block->next;

  if (links == NULL)
    LOG_E (OMG, "ERROR in reading-: NO data for node %d in this block \n",
           vid);

  while (links != NULL) {
    if (links->visit == 0)
      break;

    save = links;
    links = links->next;
  }

  if (links != NULL) {
    if (flag == DATA_AND_STATUS_CHANGE) {
      if (block->next != block->end)
        links->visit = 1;

      return links;
    } else if (flag == DATA_ONLY) {
      return save;
    }
  } else
    return links;

  DevAssert(false); // FIXME return 0?
}
