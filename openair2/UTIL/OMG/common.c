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

/*! \file OMG.c
* \brief Main function containing the OMG interface
* \author  M. Mahersi,  J. Harri, N. Nikaein,
* \date 2011
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "omg.h"

#define frand() ((double) rand() / (RAND_MAX+1))
//#ifndef STANDALONE
mapping nodes_type[] = {
  {"eNB", eNB}
  ,
  {"UE", UE}
  ,
  {NULL, -1}
};

mapping nodes_status[] = {
  {"sleeping", 0}
  ,
  {"moving", 1}
  ,
  {NULL, -1}
};

mapping mob_type[] = {
  {"STATIC", STATIC}
  ,
  {"RWP", RWP}
  ,
  {"RWALK", RWALK}
  ,
  {"TRACE", TRACE}
  ,
  {"SUMO", SUMO}
  ,
  {NULL, -1}
};

//#endif

node_struct *
create_node (void)
{
  node_struct *ptr;
  ptr = calloc (1, sizeof (node_struct));
  return ptr;
}


void
delete_node (node_struct * node)
{
  free (node->mob);
  node->mob = NULL;
  free (node);
}

double
randomgen (double a, double b)
{

  return (rand () / (double) RAND_MAX) * (b - a) + a;
}


mobility_struct *
create_mobility (void)
{
  mobility_struct *ptr;
  ptr = calloc (1, sizeof (mobility_struct));
  return ptr;
}


node_list *
add_entry (node_struct * node, node_list * node_vector_end)
{
  node_list *entry = malloc (sizeof (struct node_list_struct));
  entry->node = node;
  entry->next = NULL;

  if (node_vector_end == NULL) {
    return entry;
  } else {
    node_list *tmp = node_vector_end;
    tmp->next = entry;
  }

  return entry;
}


node_list *
remove_node_entry (node_struct * node, node_list * node_vector)
{
  node_list *list = node_vector;
  node_list *tmp, *toremove;

  if (list == NULL) {
    return NULL;
  }

  if (list->node->id == node->id) {
    // TODO delete the entry
    toremove = list;
    LOG_D (OMG, "removed entry for node %d \n", list->node->id);

    if (list->next == NULL) {
      node_vector = NULL;
      return NULL;
    } else {
      node_vector = list->next;
    }
  } else {
    while (list->next != NULL) {
      tmp = list;

      if (list->next->node->id == node->id) {
        toremove = tmp; // TODO delete the entry
        (void)toremove; /* avoid gcc warning "set but not used" */
        tmp = list->next->next;

        if (tmp != NULL) {
          list->next = tmp;
        } else {
          list->next = NULL;
        }
      }
    }
  }

  return node_vector;
}


// display list of nodes
void
display_node_list (node_list * node_vector)
{
  node_list *tmp = node_vector;

  if (tmp == NULL) {
#ifdef STANDALONE
    printf ("Empty Node_list\n");
#else
    LOG_I (OMG, "Empty Node_list\n");
#endif
  }

  while (tmp != NULL) {
    /*LOG_I(OMG,"[%s][%s] Node of ID %d is %s. Now, it is at location (%.3f, %.3f)\n",
       map_int_to_str(mob_type, tmp->node->generator),
       map_int_to_str(nodes_type, tmp->node->type),
       tmp->node->ID,
       map_int_to_str(nodes_status, tmp->node->mobile),
       tmp->node->X_pos,
       tmp->node->Y_pos ); */
    LOG_I (OMG, "[%s %d][%s] \t at (%.3f, %.3f)\n",
           map_int_to_str(nodes_type, tmp->node->type),
           tmp->node->id,
           map_int_to_str(mob_type, tmp->node->generator),
           tmp->node->x_pos, tmp->node->y_pos);

    // two options: view node mobility of one node during the entire simulation or just a snapshot of all nodes for different timestamps
    // note:gnu plot requiredifferent files for each timestamp
    //use a python script to postprocess the positions, check the

    // support: view node mobility of one node during the entire simulation OR only one snapshot
    /* TODO: this LOG_F generates a warning, below is a fix that removes a %.3f, maybe it's not the correct fix */
    /*LOG_F(OMG,"%d; %.3f; %.3f; %.3f\n",
          tmp->node->id,
          tmp->node->x_pos, tmp->node->y_pos);*/
    LOG_F(OMG,"%d; %.3f; %.3f\n",
          tmp->node->id,
          tmp->node->x_pos, tmp->node->y_pos);

    //LOG_I(OMG, "node number %d\tstatus(fix/mobile) %d\tX_pos %.2f\tY_pos %.2f\tnode_type(eNB, UE)%d\t", tmp->node->ID,tmp->node->mobile, tmp->node->X_pos,tmp->node->Y_pos, tmp->node->type);
    //LOG_D(OMG, "mob->X_from %.3f\tmob->Y_from %.3f\tmob->X_to %.3f\tmob->Y_to %.3f\t", tmp->node->mob->X_from,tmp->node->mob->Y_from, tmp->node->mob->X_to, tmp->node->mob->Y_to );
    tmp = tmp->next;
  }
}

void
display_node_position (int id, int generator, int type, int mobile, double x,
                       double y)
{
  LOG_I (OMG,
         "[%s][%s] Node of ID %d is %s. Now, it is at location (%.2f, %.2f) \n",
         map_int_to_str (mob_type, generator), map_int_to_str (nodes_type,
             type), id,
         map_int_to_str (nodes_status, mobile), x, y);
}

node_list *
filter (node_list * vector, int node_type)
{
  node_list *tmp1, *tmp2;
  tmp1 = vector;
  tmp2 = NULL;

  while (tmp1 != NULL) {
    if (tmp1->node->type == node_type) {
      tmp2 = add_entry (tmp1->node, tmp2);
    }

    tmp1 = tmp1->next;
  }

  return tmp2;
}


void
delete_node_entry (node_list * entry)
{
  node_struct *node = entry->node;
  delete_node (node);
  entry->node = NULL;
  free (entry);
}

node_list *
remove_node (node_list * list, int nid, int node_type)
{

  node_list *current = NULL, *previous = NULL;

  //int cond=0;
  //int i=0;
  if (list == NULL) {
    return NULL;
  } else {
    //start search
    current = list;

    while ((current != NULL)
           && ((current->node->id != nid)
               || (current->node->type != node_type))) {
      previous = current; //  previous hobbles after
      current = current->next;
    }

    //holds: current = NULL or  type != node_type or.., but not both
    if (current == NULL) {
      LOG_E (OMG, " Element to remove is not found\n ");
      return NULL;
    }     //value not found
    else {

      if (current == list) {
        list = current->next;
        LOG_D (OMG, "Element to remove is found at beginning\n");
      }

      else {
        previous->next = current->next;

      }

      delete_node_entry (current);  // freeing memory
      current = NULL;

    }

    return list;
  }
}

int
length (char *s)
{
  int count = 0;

  while (s[count] != '\0') {
    ++count;
  }

  return count;
}

node_struct *
find_node (node_list * list, int nid, int node_type)
{

  node_list *current;

  if (list == NULL) {
    printf (" Node_LIST for nodeID %d is NULL \n ", nid);
    return NULL;
  } else {
    //start search
    current = list;

    while ((current != NULL)
           && ((current->node->id != nid)
               || (current->node->type != node_type))) {
      current = current->next;
    }

    //holds: current = NULL or  type != node_type or.., but not both
    if (current == NULL) {
      LOG_D (OMG,
             " Element to find in Node_Vector with ID: %d could not be found\n ",
             nid);
      return NULL;
    }     //value not found
    else {
      //printf(" found a node for nodeID %d  \n ",nID);
      return current->node;
    }

    return NULL;
  }
}

node_list *
clear_node_list (node_list * list)
{
  node_list *tmp;

  if (list == NULL) {
    return NULL;
  } else {
    while (list->next != NULL) {
      tmp = list;
      list = list->next;
      delete_node_entry (tmp);
    }

    delete_node_entry (list); // clearing the last one
  }

  return NULL;
}

// TODO rewrite this part...not working correctly
node_list *
reset_node_list (node_list * list)
{
  node_list *tmp;
  //node_list *last = list;

  if (list == NULL) {
    //printf("Node_list is NULL\n");
    return NULL;
  } else {
    while (list->next != NULL) {
      tmp = list;
      list = list->next;
      tmp->node = NULL;
      //free(tmp);
    }

    list->node = NULL;  // clearing the last one | JHNOTE: dangerous here: the pointer is not NULL, but node is...that leads to segfault...
    //free(last);
  }

  return list;
}

// TODO rewrite this part...not working correctly
string_list *
clear_string_list (string_list * list)
{
  string_list *tmp;

  if (list == NULL) {
    return NULL;
  } else {
    while (list->next != NULL) {
      tmp = list;
      list = list->next;
      free (tmp->string);
      tmp->string = NULL;
      free (tmp);
    }

    list->string = NULL;  // clearing the last one | JHNOTE: dangerous here: the pointer is not NULL, but node is...that leads to segfault...
  }

  return list;
}

#ifdef STANDALONE
/*
 * for the two functions below, the passed array must have a final entry
 * with string value NULL
 */
/* map a string to an int. Takes a mapping array and a string as arg */
int
map_str_to_int (mapping * map, const char *str)
{
  while (1) {
    if (map->name == NULL) {
      return (-1);
    }

    if (!strcmp (map->name, str)) {
      return (map->value);
    }

    map++;
  }
}

/* map an int to a string. Takes a mapping array and a value */
char *
map_int_to_str (mapping * map, int val)
{
  while (1) {
    if (map->name == NULL) {
      return NULL;
    }

    if (map->value == val) {
      return map->name;
    }

    map++;
  }
}





#endif
