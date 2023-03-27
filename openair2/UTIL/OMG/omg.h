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

/*! \file OMG.h
* \brief Prototypes of OMG APIs
* \author  M. Mahersi, N. Nikaein, J. Harri
* \date 2011 - 2014
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#ifndef __OMG_H_
#define __OMG_H_
#include "omg_constants.h"
#include "defs.h"
#include "omg_vars.h"

#if STANDALONE
#define LOG_G(c, x...) printf(x)
#define LOG_A(c, x...) printf(x)
#define LOG_C(c, x...) printf(x)
#define LOG_E(c, x...) printf(x)
#define LOG_W(c, x...) printf(x)
#define LOG_N(c, x...) printf(x)
#define LOG_I(c, x...) printf(x)
#define LOG_D(c, x...) printf(x)
#define LOG_F(c, x...) printf(x)
#define LOG_T(c, x...) printf(x)

#else
#include "UTIL/LOG/log.h"


#endif

/** @defgroup _omg OpenAir Mobility Generation (OMG)
The area of vehicular ad hoc networks (\b VANETs) is a fast evolving research area. Attracting a lot the attention of researchers, it is likely to become the most relevant form of mobile ad hoc networks in the soon coming years. Because VANETs usually comprise a great number of cars, a real world test is a very costly and time consuming operation. Therefore, most VANET research is carried out using simulations thanks to abstractions or mobility models.\n
Given the great importance of mobility features in the world of wireless communication, adding a \b mobility \b generator to the emulator OpenAir is absolutely useful especially that it allows experimenting innovative approaches in a realistic radio propagation and application scenario.
*/


/** @defgroup _omg_ OMG: The mobility generator of OpenAirInterface
 *  @ingroup _omg
OMG, or OpenAir Mobility Generation has born to add the mobility features to OpenAir given the increasing importance of realistic mobility patterns in the world of wireless networks and the need for simulation to explore this area. OMG includes the most used mobility models, in three classes, namely: \b EMBEDDED \b TRACE \ FEDERATED. In the \b EMBEDDED class, random mobility is embedded in OMG. The most used random models have been implemneted, namely \b Random \b Walk , \b Random \b Way \b Point and \b Graph based mobility. The \b TRACE class is able to load into OMG and OAI trace-based mobility files. The \b FEDERATED class allows OMG to interface with an external mobility/traffic generator to obtain and change mobility patterns. The \b FEDERATED class currently support the federation with SUMO, Simulation of Urban Mobility, but could easily be extended to accept other simulator. Thus, it is capable of abstracting the motion of vehicles or mobile nodes in different ways. And since it is also useful to model the fix nodes that can exist in the road to communicate with the mobile nodes, a \b STATIC model is also included in OMG. \n
OMG has a direct relation ship with the (edit: deleted as of 2023-03-09) OCG module (OpenAir Config Generation). Actually, this latter module is responsible of setting the emulation time to synchronize OMG, requesting OMG to start simulation scenario, update the nodes positions and many other features. (see section Specification) \n
*/


/** @defgroup _mob_models Random Mobility Models: Brief Overview
 *  @ingroup _omg
 Mobility models have been developed to simulate the real world adhoc mobile networks in order to determine whether the proposed solutions will be useful and efficient if implemented. In the literature, one way of classifying the mobility models is based on the nodes interaction. We distinguish between:
 - Entity mobility models: \n In these models, the nodes actions are completely independent of each other. Random models such as Random Way Point and Random Walk are the most common models used by researchers.\n

 - Group mobility models: In this case, the nodes’ decision on movement deeply depends on the other nodes’ movement in the group. Reference Point Mobility Group Model is an example of Group mobility models. It represents the network as a set of groups, where a group leader decides for the group’s movement pattern (destination, speed, etc).
 */



/** @defgroup _rwp_mob Random Way Point Mobility Model
 *  @ingroup _mob_models

According to this model, a mobile node starts by staying in one location for a certain period of time, called "\e pause \e time" and chosen randomly.  Once the pause time expires, the node chooses a \e random \e destination inside the simulation area as well as a \e random \e speed.
The speed is supposed to be constant during the journey time needed to reach the destination location. Upon arrival, the vehicle pauses again before choosing a new destination and restarting the same described process again. \n
\e Pause \e time, \e speed and \e destination \e location are chosen randomly between minimum and maximum values in order to control the general behavior of the network. For example, choosing slow vehicles with large pause times results on a very stable network, where links and topology hardly change. \n
Random Way Point model’s popularity comes from its simple tenet. Randomness is in many cases sufficient to simulate the motion of vehicles.
 ....................figure
*/


/** @defgroup _rwalk_mob Random Walk Mobility Model
 *  @ingroup _mob_models

In Random Walk mobility models, a mobile node moves through the simulation area by randomly choosing a \e speed and a direction or \e azimuth. When it reaches the new location, it restarts the same process by choosing the next speed and azimuth from predefined ranges, i.e. [\e min_speed, \e max_speed] and [\e 0, \e 2p] respectively, and so on. \n
But what if the destination position, computed according to the chosen azimuth and speed exceeds the simulation area boundaries?  \n
Many solutions are proposed in this case. For instance, the destination position can simply be set to the maximum (or minimum) boundary. In other words, if the calculated X coordinate, for example, exceeds the maximum allowed X coordinate, it is thus set to this maximum value of X. Another possible approach is to make the mobile node, leaving the simulation area, re-enter from the opposite side. \n
Thanks to its simplicity of implementation, Random Walk is a widely used mobility model in simulations. \n
....................figure

 */

/** @defgroup _trace_mob Trace-based Mobility Model
 *  @ingroup _mob_models

The trace-based mobility model allows a user to provide external mobility traces, eithe gathered from realistic monitoring, from an different simulator or specififed according to particular scenarios. The trace format follows that of <TIME> <ID> <X> <Y> <SPEED>, but new parser could easily be added. \n
Node that this requires the user to specify in OCG the name of the trace file, which should be located user the ./TRACE folder. \n
....................figure

 */

/** @defgroup _sumo_mob SUMO Mobility Model
 *  @ingroup _mob_models

When professional and close-to-realisty mobility patterns are required, in paticular considering pedestrian, public transportation or private vehicles in urban areas, the simulator SUMO is a popular choice. Accordingly, OMG interfaces itself with SUMO using the OMG TraCI interface and exchange commands and mobility informations via sockets. OMG holds a ID manager to map nodes ID from OAI and nodes ID from SUMO. When SUMO is used, OMG does not have any job related to mobility as it delegates it to SUMO.\n
....................figure

 */


/** @defgroup _mob_models_in_openair Mobility Models in OpenAir
 *  @ingroup _omg
 */


/** @defgroup _specification Specification
 *  @ingroup _mob_models_in_openair

The aim is to implement, in \e C \e language, a code that enables to record the motion of the nodes in the simulation area during the time. The nodes can be static or mobile and can be of different types such as \e UE, \e eNB, \e GW, etc. We also need to be able to change nodes mobility models during the simulation or to restart a new simulation scenario.
The developed code is to be integrated on OpenAir as the OMG part or OpenAir Mobility Generation.  \n
 As the following figure shows, OMG is "handled" by OCG and acts according to its requests. \n
\image random_init.png
As we can see, OCG is responsible of setting the emulation time to synchronize OMG. Moreover, it is in charge of starting the simulation scenario and passing the needed parameters. OCG can ask for updating the nodes positions as time goes by. It can also request the current locations of all or specific type of nodes moving according to a given mobility model. It is as well possible to request the current position of a specific node, identified by its type and identifier. Furthermore, OCG can request changing the mobility generator of an existing node during the simulation in order to meet the needs of a specific application.
 */


/** @defgroup _design Design
 *  @ingroup _mob_models_in_openair
In order to carry out the functionnalities described in the specification, the following main principles are suggested:
 - OCG starts by setting the needed parameters, by default or according to the user’s choice. The emulation time, the size of the simulation area, the ranges of X and Y coordinates, speed, sleep duration and journey time are the main passed parameters.

 - OCG asks OMG to initialize the simulation scenario by placing the nodes in the simulation area taking into account the passed parameters and according to the specified node type and mobility model.

 - After initialization, OMG manages to move the non static nodes in accordance with the chosen mobility model principles. To do so, it handles a Job_Vector that gathers the "jobs" of all the existing mobile nodes. Actually, to each non static node is associated a job, characterized by a specific execution time and which can be either a move or a sleep in the context of OMG. Thanks to OCG call for update each unit of time, OMG is able to keep track of emulation time and thus execute each job at its correct time.

 - OMG handles also a Node_Vector per mobility model in order to store the useful data about each node such as the ID, speed, type and the past, current and next location, etc. The Node_Vector is updated with each new job execution in order to always reflect the correct information about the nodes. It is consequently too simple to return the current locations of the nodes if asked to do so by OCG.  And even if the node which we want to know the position is still moving, OMG can compute its current position by interpolation so that the correctness of the result is guaranteed.

 - If asked to change the mobility generator of a node, OMG starts by checking whether this node exists. Then, given the ID and the type of the node, it is easy for OMG to look for the node in the Node_Vectors and change its mobility generator. It is obviously indispensable to perform other operations in order to update the Job_Vector and the concerned Node_Vectors according to the new information.

 */


/** @defgroup _code Code
 *  @ingroup _mob_models_in_openair
See Code ??????????????????????????????????????????
 */



/** @defgroup _results_eval Results & Evaluation
 *  @ingroup _mob_models_in_openair
In order to check the conformity of OMG behavior with the specification, we start by checking the randomness of the generated initial locations of the nodes, since it is unrealistic and useless to have juxtaposed nodes. The following figure shows the locations obtained in the first phase of initialization. As we can see, OMG allows representing nodes of multiple types and mobility generators and placing them randomly in the simulation area. \n
…. fig init \n

Furthermore, we check the correctness of the positions returned by OMG for each mobility model. For this purpose, we can keep track of a specific node thanks to its ID and type. We then compare the expected locations with the ones returned by OMG. We notice that OMG results are consistent with the expected ones.
 */



/** @defgroup _conclusion_ Conclusion
 *  @ingroup _omg
OMG is a new module of OpenAirInterface that has been created to deal with the mobility issues in the context of VANETs. At present, it is able to model the two most popular entity random mobility models, namely Random Way Point and Random Walk, but it has a supple and generic structure that allows to easily take on more mobility models. Mainly, OMG allows to record the motion of the mobile nodes within the simulation area and instantiates also the static nodes, permitting therefore to model a genuine vehicular network. \n
Based on a simple C code, the efficiency of OMG is reinforced by its great flexibility. It is consequently mere to add more sophisticated and accurate mobility models to it. Indeed, a new model, called \b Grid \b Walk is presently under preparation.
 */







/**
 * \fn void init_omg_global_params(void)
 * \brief Useful function in case we need to re-iniltialize all the Node_Vectors as well as the Job_Vector. It is a kind of reset function
 */
void init_omg_global_params(void);


/**
 * \fn void init_mobility_generator(omg_global_param omg_param_list)
 * \brief Assign initial positions to all the nodes. To do so, call the start_generator..... function corresponding to each requested mobility generator
 * \param omg_param_list a structure that contains the main parameters needed to establish the random positions distribution
 */
void init_mobility_generator(omg_global_param omg_param_list[]);

/**
 * \fn void stop_mobility_generator(int mobility_type)
 * \brief Call the destructor for the respective mobility type.
 * \param int mobility_type to stop and call destructor
 */
void stop_mobility_generator(omg_global_param* omg_param_list);

/**
 * \fn void update_nodes(double cur_time)
 * \brief Update the positions of all the nodes, i.e all the non empty Node_Vectors. It calls update_node_vector(int mobility_type, double cur_time) for each non NULL Node_Vector
 * \param cur_time a variable of type double that represents the current time
 */
void update_nodes(double cur_time);


/**
 * \fn Node_list get_current_positions(int mobility_type, int node_type, double cur_time)
 * \brief According to the mobility type, call the corresponding get_positions_updated... function in order to compute the intermediate positions of the nodes
 * \param mobility_type an integer that represents the mobility model name
 * \param node_type a variable of type integer that represents the type of nodes that we want to get the current positions
 * \param cur_time a variable of type double that represents the current time
 * \return a Node_list structure that stores the basic information about the all the nodes of a given mobility model
 */
node_list* get_current_positions(int mobility_type, int node_type, double cur_time);

void get_nodes_positions (int mobility_type, double cur_time);

/**
 * \fn void set_new_mob_type(int nID, int node_type, int new_mob, double cur_time)
 * \brief Given the ID of the node and its type, change its correponding mobility generator into the one given as argument: new_mob
 * \param nID an integer that represents the ID of the target node
 * \param node_type an integer that reflects the node's type
 * \param new_mob an integer that represents the new mobility generator
 * \param cur_time a variable of type double that represents the current time, i.e the time of mobility generator change
 */
void set_new_mob_type(int nID, int node_type, int new_mob, double cur_time);




#endif /* __OMG_H_ */
