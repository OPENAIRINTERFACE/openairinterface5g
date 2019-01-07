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

/*! \file communicationthread.cpp
* \brief this thread is to process the communication between the simulator and the visualisor
* \author M. Mosli
* \date 2012
* \version 0.1 
* \company Eurecom
* \email: mosli@eurecom.fr
*/ 

#include "communicationthread.h"

typedef struct {
    char *name;	/*!< \brief string name of item */
    int value;	/*!< \brief integer value of mapping */
} mapping;

mapping omg_model_names[] = {
  {"STATIC", 0},
  {"RWP", 1}, 
  {"RWALK", 2},
  {"TRACE", 3}, 
  {"SUMO", 4}, 
  {NULL, -1}
};

mapping state_model_names[] = {
  {"NOT_SYNC", 0},
  {"SYNCED", 1}, 
  {"CONNECTED", 2},
  {"ATTACHED", 3}, 
  {"DATA_COMMUNICATION", 4}, 
  {NULL, -1}
};


/* map an int to a string. Takes a mapping array and a value */
char *map_int_to_str(mapping *map, int val) {
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
// mobtype = map_int_to_str(omg_model_names,omv_data.geo[i].mobility_type)

extern int pfd[2];
extern struct Geo geo[NUMBER_OF_eNB_MAX+NUMBER_OF_UE_MAX];
extern int nb_frames;
extern int node_number;
extern int nb_enb;
extern int nb_ue;
extern int nb_antennas_rx;

CommunicationThread::CommunicationThread(MyWindow* window){
    this->window = window;
    QObject::connect(this, SIGNAL(newData(QString, int)), window, SLOT(writeToConsole(QString, int)));
    QObject::connect(this, SIGNAL(newPosition()), window->getGL(), SLOT(drawNewPosition()));
    QObject::connect(this, SIGNAL(endOfTheSimulation()), window, SLOT(endOfTheSimulation()));
}

void CommunicationThread::run()
{
    int nread;
    int end = 0;
    int counter = nb_frames;
    Data_Flow_Unit data;

    while (counter > 0 && !end){
        switch( nread = read(pfd[0], &data, sizeof(Data_Flow_Unit))) {
        case -1 :
	    if (counter < nb_frames)
		end = 1;
            perror( "read" );
            break;
        case 0 :
	    if (counter < nb_frames)
		end = 1;
            perror( "EOF" );
            break;
        default :
            QString information, information_for_a_node, format_information;
	    geo[nb_enb].mobility_type = data.geo[nb_enb].mobility_type;
	    information.sprintf("\nFrame: %d\n\n",nb_frames - counter);
	    
	    format_information.sprintf("Format of Log: (<X> is X's value)\nFor eNb :: [eNb <Id>] at <Mob_type>(<Position>) with <nb_connected_ue> connected UE\nFor UE :: [UE <Id>][RNTI <rnti>][state <state>] at <Mob_type>(<Position>), PL=<Pathloss>, RSRP=<rsrp>, RSRQ=<rsrq>, from eNb <connected eNb>\n==========================================================\n");
         
	    information += format_information;
	    
	    end = data.end;
            
            for (int i = 0; i<nb_enb; i++){

                geo[i].Neighbors = data.geo[i].Neighbors;
                geo[i].x = data.geo[i].x;
		geo[i].y = data.geo[i].y;
		geo[i].node_type = data.geo[i].node_type;
		geo[i].mobility_type = data.geo[i].mobility_type;

		information_for_a_node.sprintf("[eNb %d] at %s(%d,%d) with %d connected UE\n" , i, 
						map_int_to_str(omg_model_names, geo[i].mobility_type),
						  data.geo[i].x, data.geo[i].y,
						  data.geo[i].Neighbors     );
		information += information_for_a_node;		

                for(int j = 0; j<data.geo[i].Neighbors; j++)
                    geo[i].Neighbor[j] = data.geo[i].Neighbor[j];
            }
	    
	    QString state_str;
	    
            
	    for (int i = nb_enb; i<node_number; i++){
	
		geo[i].Neighbors = data.geo[i].Neighbors;
                geo[i].x = data.geo[i].x;
		geo[i].y = data.geo[i].y;
		geo[i].node_type = data.geo[i].node_type;
		geo[i].state = data.geo[i].state;
		geo[i].connected_eNB = data.geo[i].connected_eNB;
		geo[i].rnti = data.geo[i].rnti;
		geo[i].RSRP = data.geo[i].RSRP;
		geo[i].RSRQ = data.geo[i].RSRQ;
		geo[i].Pathloss = data.geo[i].Pathloss;
		geo[i].mobility_type = data.geo[i].mobility_type;
	
		for (int ant = 0; ant < nb_antennas_rx; ant++){
		  geo[i].RSSI[ant] = data.geo[i].RSSI[ant];
		}
		
		information_for_a_node.sprintf("[UE %d][RNTI %d][state %s] at %s(%d,%d), PL=%d, RSRP=%d, RSRQ=%d, from eNb %d\n" , 
						i - nb_enb, geo[i].rnti,map_int_to_str(state_model_names, geo[i].state),
						map_int_to_str(omg_model_names, geo[i].mobility_type),
					       geo[i].x, geo[i].y,
					      geo[i].Pathloss, geo[i].RSRP, geo[i].RSRQ, 
					      geo[i].connected_eNB);
		
		information += information_for_a_node;		

                for(int j = 0; j<data.geo[i].Neighbors; j++)
                    geo[i].Neighbor[j] = data.geo[i].Neighbor[j];
            }

            emit newData(information, nb_frames - counter);
            emit newPosition();
            break;
        }
        counter--;
    }

    sleep(30);
    emit endOfTheSimulation();

    

/*
    switch(nread = read(pfd[0], &final, sizeof(Final_Data))){
        case -1 :
            perror( "read" );
            break;
        case 0 :
            perror( "EOF" );
            break;
        default :
            QString final_data;
            final_data.sprintf("\n*** Final results ***\n\n* Node number: %d\n* Loss rate: %f\n* All forwarded packets: %d\n* Used memory [device, host]: [%d, %d]\n* Elapsed time (average): %f\n* Elapsed time (min): %f\n* Elapsed time (max): %f\n",final.node_number, final.loss, final.forwarded,final.device_memory, final.host_memory,final.average_time, final.min_time, final.max_time);
            emit newData(final_data);
            emit endOfTheSimulation();
            break;
    }
*/
}
