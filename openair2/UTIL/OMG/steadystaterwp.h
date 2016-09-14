/*! \file steadystaterwp.h
* \brief random waypoint mobility generator
* \date 2014
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/
#ifndef STEADYSTATERWP_H_
#define STEADYSTATERWP_H_


int start_steadystaterwp_generator (omg_global_param omg_param_list);

void place_steadystaterwp_node (node_struct* node);

void sleep_steadystaterwp_node (pair_struct* pair, double cur_time);

void move_steadystaterwp_node (pair_struct* pair, double cur_time);

double pause_probability(omg_global_param omg_param);


double initial_pause(omg_global_param omg_param);

double initial_speed(omg_global_param omg_param);

void update_steadystaterwp_nodes (double cur_time);

void get_steadystaterwp_positions_updated (double cur_time);

#endif











