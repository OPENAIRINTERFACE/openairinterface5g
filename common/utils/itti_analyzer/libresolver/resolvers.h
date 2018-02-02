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

#ifndef RESOLVERS_H_
#define RESOLVERS_H_

int resolve_typedefs(types_t **head);

int resolve_struct(types_t **head);

int resolve_pointer_type(types_t **head);

int resolve_field(types_t **head);

int resolve_array(types_t **head);

int resolve_reference(types_t **head);

int resolve_union(types_t **head);

int resolve_file(types_t **head);

int resolve_function(types_t **head);

int search_file(types_t *head, types_t **found, int id);

#endif /* RESOLVERS_H_ */
