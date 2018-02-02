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

/*! \file omv.cpp
* \brief openair mobility visualisor
* \author M. Mosli
* \date 2012
* \version 0.1 
* \company Eurecom
* \email: mosli@eurecom.fr
*/ 

#include "mywindow.h"
#include <stdlib.h>
#include <sys/stat.h>
#include "communicationthread.h"


//pid_t simulator_pid;
int pfd[2];
struct Geo geo[NUMBER_OF_eNB_MAX+NUMBER_OF_UE_MAX];
int x_area, y_area, z_area;
int nb_frames;
int nb_enb;
int nb_ue;
int node_number;
char frame_format[10];
int tdd_configuration;
extern int nb_antennas_rx;

CommunicationThread* communication_thread;

void init_parameters(int frames, int num_enb, int num_ue, float x, float y, float z, int nb_antenna, char* frame_config, int tdd_config){
  x_area = (x == 0) ? 1 : (int) x;
  y_area = (y == 0) ? 1 : (int) y;
  z_area = (z == 0) ? 1 : (int) z;
  nb_enb = num_enb;
  nb_ue = num_ue;
  node_number = nb_enb + nb_ue;
  nb_frames = frames;
  nb_antennas_rx = nb_antenna;
  strcpy(&frame_format[0], frame_config);
  tdd_configuration= tdd_config;
}

int main(int argc, char *argv[]) {
  
  if (argc <= 1)
    init_parameters(100,1, 5, 5000.0,5000.0,1000.0, 4,"TDD", 3);
  else {
    init_parameters(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atof(argv[5]),atof(argv[6]),atof(argv[7]),atoi(argv[8]), argv[9], atoi(argv[10]));
    pfd[0]= atoi(argv[1]);
  }
  QApplication app(argc, argv);
  
  MyWindow window;
  window.move(*(new QPoint(150,100)));
  window.show();
  
  int end_value = app.exec();
  
  return end_value;
}
