/**
 * \file static.h
 * \brief Prototypes of the functions used for the STATIC model
 * \date 22 May 2011
 *
 */

#ifndef STATIC_H_
#define STATIC_H_

#include "omg.h"

/**
 * \fn void start_static_generator(omg_global_param omg_param_list)
 * \brief Start the STATIC model by setting the initial position of each node
 * \param omg_param_list a structure that contains the main parameters needed to establish the random positions distribution
 */
void start_static_generator(omg_global_param omg_param_list);


/**
 \fn void place_static_node(NodePtr node)
 * \brief Generates a random position ((X,Y) coordinates) and assign it to the node passed as argument. This latter node is then added to the Node_Vector[STATIC]
 * \param node a pointer of type NodePtr that represents the node to which the random position is assigned
 */
void place_static_node(node_struct* node);

#endif /* STATIC_H_ */
