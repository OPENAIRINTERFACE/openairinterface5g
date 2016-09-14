/**
 * \file grid.h **/

#ifndef GRID_H_
#define GRID_H_

#include "omg.h"

int max_vertices_ongrid(omg_global_param omg_param);

int max_connecteddomains_ongrid(omg_global_param omg_param);


double vertice_xpos(int loc_num, omg_global_param omg_param);


double vertice_ypos(int loc_num, omg_global_param omg_param);


double area_minx(int block_num, omg_global_param omg_param);


double area_miny(int block_num, omg_global_param omg_param);

unsigned int next_block(int current_bn, omg_global_param omg_param);

unsigned int selected_blockn(int block_n,int type,int div);

#endif

