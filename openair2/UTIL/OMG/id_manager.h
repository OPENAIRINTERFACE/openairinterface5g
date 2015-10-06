/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file id_manager.h
* \brief Prototypes of the OMG ID Manager to handle the mapping between OAI ID and SUMO ID
* \author  J. Harri
* \date 2012
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#ifndef __IDMANAGER_H_
#define __IDMANAGER_H_

#include <stdlib.h>

/*!A sructure that represents a mapping (OAI_ID, SUMO_ID), i.e the node ID given by OAI and the node ID given by SUMO */
typedef struct map_struct {
  int oai_id; /*!< the oai ID as an integer */
  char *sumo_id;  /*!< SUMO gives IDs as strings */
} map_struct;

typedef struct map_struct* MapPtr;  /*!< The typedef that reflects a #map_struct*/

/*!A sructure that gathers all the existing ID Maping */
struct map_list_struct {
  map_struct *map;  /*!< Avariable of type #MapPtr. It represents a node */
  struct map_list_struct  *next; /*!< A pointer to the next element */
} map_list_struct;

typedef struct map_list_struct* Map_list; /*!< The typedef that reflects a #map_list_struct*/

/*!A sructure that includes all characteristics of an ID management Unit  */
typedef struct id_manager_struct {
  struct map_list_struct *map_oai2sumo;  /*!< Avariable of type #MapPtr. It represents the mapping from OAI ID to SUMO ID for one noe */
  struct map_list_struct *map_sumo2oai;  /*!< Avariable of type #MapPtr. It represents the mapping from SUMO ID to OAI ID for one noe */

} id_manager_struct;

typedef struct id_manager_struct* IDManagerPtr; /*!< The typedef that reflects a #id_manager_struct*/


/**
 * \fn MapPtr create_map(void)
 * \brief Create and allocate memory for a map object (OAI ID / SUMO ID).
 * \return a pointer to the newly created Map
 */
MapPtr create_map(void);

/**
 * \fn IDManangerPtr create_IDManager(void)
 * \brief Create and allocate memory for a the ID Manager.
 * \return a pointer to the ID Manager
 */
IDManagerPtr create_IDManager(void);

/**
 * \fn Map_list add_entry(MapPtr map, Map_list Map_Vector)
 * \brief Add a mapping entry to the MapList
 * \param map the pointer to the map to be added to the Map_list
 * \param Map_Vector the reference to the Map_list (pointing to the HEAD if not NULL)
 * \retun the pointer to the updated/created map list (pointing to the HEAD)
 */
Map_list add_map_entry(MapPtr map, Map_list Map_Vector);

/**
 * \fn char* get_SumoID_by_OAI(int oai_id, IDManagerPtr ID_manager)
 * \brief map a SUMO ID from a OAI ID; return NULL if the mapping does not exist
 * \param oai_id the OAI ID to be mapped to the SUMO ID
 * \param ID_manager the reference to the ID Manager
 * \return the SUMO ID in string format
 */
char* get_sumoID_by_OAI(int oai_id, IDManagerPtr ID_manager);

/**
 * \fn int get_OaiID_by_SUMO(char* sumo_id, IDManagerPtr ID_manager)
 * \brief map a OAI ID from a SUMO ID; return -1 if the mapping does not exist
 * \param sumo_id the SUMO ID to be mapped to the OAI ID
 * \param ID_manager the reference to the ID Manager
 * \return the OAI ID in int format
 */
int get_oaiID_by_SUMO(char* sumo_id, IDManagerPtr ID_manager);

/**
 * \fn int remove_OaiID_by_SUMO(char *sumo_id, IDManagerPtr ID_manager);
 * \brief remove an OAI-SUMO mapping from a SUMO ID, and return the ID; return -1 if the mapping does not exist
 * \param sumo_id the SUMO ID used to remove the mapping
 * \param ID_manager the reference to the ID Manager
 * \return the updated pointer to the Map_list
*/
void remove_oaiID_by_SUMO(char *sumo_id, IDManagerPtr ID_manager);

/**
 * \fn char* get_sumo_entry(int oai_id, Map_list Map_Vector)
 * \brief retrieve the SUMO ID from a OAI ID from the OAI_SUMO map; return NULL if not found
 * \param oai_id the OAI ID to retrive the corresponding SUMO ID
 * \param Map_Vector the reference to the OAI_2_SUMO Map list
 * \return the SUMO ID in string format
 */
char* get_sumo_entry(int oai_id, Map_list Map_Vector);

/**
 * \fn get_oai_entry(char* sumo_id, Map_list Map_Vector)
 * \brief retrieve the OAI ID from a SUMO ID from the SUMO_OAI map; returns -1 if not found
 * \param sumo_id the SUMO ID to retrieve the corresponding OAI ID
 * \param Map_Vector the reference to the SUMO_2_OAI Map list
 * \return the OAI ID in int format
 */
int get_oai_entry(char*, Map_list);


/**
 * \fn int remove_oai_entry(char *sumo_id, Map_list Map_Vector);
 * \brief remove the SUMO-OAI mapping, from a SUMO ID; returns the found OAI ID; returns -1 if the mapping does not exist.
 * \param sumo_id the SUMO ID used to remove the mapping.
 * \param Map_Vector the reference to the SUMO_2_OAI Map list
 * \return the updated pointer to the Map_list
 */
Map_list remove_oai_entry(char *sumo_id, Map_list Map_Vector);



#endif /* __IDMANAGER_H_ */
