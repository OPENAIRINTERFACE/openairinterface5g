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

/*! \file mywindow.cpp
* \brief manages the window and its components
* \author M. Mosli
* \date 2012
* \version 0.1 
* \company Eurecom
* \email: mosli@eurecom.fr
*/ 

#include "mywindow.h"
#include "communicationthread.h"
#include <math.h>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSlider>
#include <QString>
#include <QGroupBox>
#include <QIcon>
#include <QPixmap>
#include <QSpinBox>
#include <QFormLayout>
#include <QPalette>
#include <QFileDialog>

//extern pid_t simulator_pid;
extern int pfd[2];
extern struct Geo geo[NUMBER_OF_eNB_MAX+NUMBER_OF_UE_MAX];
extern CommunicationThread* communication_thread;
extern int x_area, y_area;
extern int nb_frames;
extern int nb_enb;
extern int nb_ue;
extern char frame_format[10];
extern int tdd_configuration;

int supervised_id = 0;
int nb_antennas_rx;
QString uri;

MyWindow::MyWindow() : QWidget()
{
    pattern = 0;
    
    this->setFixedSize(940, 820);
    QObject::connect(this, SIGNAL(exitSignal()), qApp, SLOT(quit()));
    
    /* Generic area widgets */
    
    QString sim_data;
    QLabel *area = new QLabel, *nbr_enb = new QLabel, *nbr_ue = new QLabel, *nbr_frames = new QLabel, 
           *frame_format_label = new QLabel, *tdd_label = new QLabel;
    
    QGroupBox *generic = new QGroupBox("Generic Information", this);
    QFormLayout *generic_layout = new QFormLayout;
    generic_layout->setVerticalSpacing(10);
    generic_layout->setHorizontalSpacing(20);
    generic_frame = new QLabel("0");
    
    sim_data.sprintf("<html>%dm x %dm</html>", x_area, y_area);
    area->setText(sim_data);
  
    sim_data.sprintf("<html>%d</html>", nb_enb);
    nbr_enb->setText(sim_data);
  
    sim_data.sprintf("<html>%d</html>", nb_ue);
    nbr_ue->setText(sim_data);
    
    sim_data.sprintf("<html>%d</html>", nb_frames);
    nbr_frames->setText(sim_data);
    
    sim_data.sprintf("<html>%s</html>", frame_format);
    frame_format_label->setText(sim_data);
    
  
    
    sim_data.sprintf("<html>%d</html>", tdd_configuration);
    tdd_label->setText(sim_data);
    

    generic_layout->addRow(new QLabel("<html><u>Area dimensions:</u></html>"), area);
    generic_layout->addRow(new QLabel("<html><u>eNb node number:</u></html>"), nbr_enb);
    generic_layout->addRow(new QLabel("<html><u>UE node number:</u></html>"), nbr_ue);
    generic_layout->addRow(new QLabel("<html><u>Nb frames:</u></html>"), nbr_frames);
    generic_layout->addRow(new QLabel("<html><u>Frame format:</u></html>"), frame_format_label);
    generic_layout->addRow(new QLabel("<html><u>TDD configuration:</u></html>"), tdd_label);
    generic_layout->addRow(new QLabel("<html><FONT COLOR='blue' >Frame:</FONT></html>"), generic_frame);
   
    generic->setLayout(generic_layout);

    /* Control widgets */
    QCheckBox *drawConnections = new QCheckBox("Draw Connections");
    drawConnections->setChecked(true);
    QObject::connect(drawConnections, SIGNAL(stateChanged(int)), this, SLOT(setDrawConnections(int)));
    
    QCheckBox *useMap = new QCheckBox("Use a Map");
    useMap->setChecked(true);
    QObject::connect(useMap, SIGNAL(stateChanged(int)), this, SLOT(setUseMap(int)));
    
    QComboBox *nodes_color = new QComboBox(this);
    QComboBox *links_color = new QComboBox(this);
    used_map = new QComboBox(this);
    QHBoxLayout *nodes_color_layout = new QHBoxLayout(this);
    QHBoxLayout *links_color_layout = new QHBoxLayout(this);
    QHBoxLayout *used_map_layout = new QHBoxLayout(this);
    QLabel *nodes_color_label = new QLabel;
    QLabel *links_color_label = new QLabel;
    QLabel *used_map_label = new QLabel("Used map ");
    nodes_color_label->setText("Nodes color ");
    links_color_label->setText("Links color ");
    QFrame *node_color_frame = new QFrame;
    QFrame *link_color_frame = new QFrame;
    QFrame *used_map_frame = new QFrame;
    
    
    QString image;
    uri.sprintf("%s/UTIL/OMV/",getenv("OPENAIR2_DIR"));
    
    used_map->setIconSize(* (new QSize(20,20)));
    
    image = "jpg.jpeg";
    used_map->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Import map");
    
    image = "mus.png";
    used_map->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Mus");
    
    image = "new.jpg";
    used_map->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Trefoil");
    
    image = "new2.jpg";
    used_map->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Ocean");
    
    image = "white.png";
    used_map->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   White Bg");
    
    image = "red.png";
    used_map->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Red Bg");
    
    image = "green.png";
    used_map->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Green Bg");
    
    image = "blue.png";
    used_map->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Blue Bg");
    QObject::connect(used_map, SIGNAL(activated(int)), this, SLOT(setUsedMap(int)));
    
    nodes_color->setIconSize(* (new QSize(30,15)));
    
    image = "red.png";
    nodes_color->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Red");
    
    image = "blue.png";
    nodes_color->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Blue");
    
    image = "green.png";
    nodes_color->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Green");
    
    image = "white.png";
    nodes_color->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   White");
    QObject::connect(nodes_color, SIGNAL(currentIndexChanged(int)), this, SLOT(setNodesColor(int)));

    links_color->setIconSize(* (new QSize(30,15)));
    
    image = "white.png";
    links_color->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   White");
    
    image = "red.png";
    links_color->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Red");
    
    image = "blue.png";
    links_color->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Blue");
    
    image = "green.png";
    links_color->addItem(*(new QIcon (* (new QPixmap(uri + image)))),"   Green");
    QObject::connect(links_color, SIGNAL(currentIndexChanged(int)), this, SLOT(setLinksColor(int)));
    
    
    QFrame *node_size = new QFrame(this);
    QLabel *nodes_size_label = new QLabel("<html><b>Size:</b></html>");
    QLabel *min = new QLabel("Small");
    QLabel *max = new QLabel("Big");
    QSlider* size = new QSlider;
    size->setMinimum(1);
    size->setMaximum(3);
    size->setValue(2);
    size->setOrientation(Qt::Horizontal);
    QObject::connect(size, SIGNAL(sliderMoved(int)), this, SLOT(updateSize(int)));
    QHBoxLayout *size_layout = new QHBoxLayout;
    size_layout->addWidget(nodes_size_label);
    size_layout->addWidget(min);
    size_layout->addWidget(size);
    size_layout->addWidget(max);
    node_size->setLayout(size_layout);
    
    
    nodes_color_layout->addWidget(nodes_color_label);
    nodes_color_layout->addWidget(nodes_color);
    used_map_layout->addWidget(used_map_label);
    used_map_layout->addWidget(used_map);
    links_color_layout->addWidget(links_color_label);
    links_color_layout->addWidget(links_color);
    node_color_frame->setLayout(nodes_color_layout);
    link_color_frame->setLayout(links_color_layout);
    used_map_frame->setLayout(used_map_layout);
    
    
    /* Specific node widgets */
    QSpinBox* id_choice = new QSpinBox;
    id_choice->setRange(0,nb_ue - 1);
    QObject::connect(id_choice, SIGNAL(valueChanged(int)), this, SLOT(updateSupervNode(int)));
   
    
    QGroupBox *specific = new QGroupBox("Specific Information", this);
    QFormLayout *specific_layout = new QFormLayout;
    specific_layout->setVerticalSpacing(10);
    specific_layout->setHorizontalSpacing(40);
    
    specific_position = new QLabel("(0,0)");
    specific_state = new QLabel("Attached");
    specific_dist = new QLabel("100");
    specific_connected_enb = new QLabel("0");
    specific_pathloss = new QLabel("0");
    specific_rnti = new QLabel("0");
    specific_rsrp = new QLabel("0");
    specific_rsrq = new QLabel("0");
    
    rssi_tab = new QLabel[nb_antennas_rx];
    //ant_tab = new QLabel[nb_antennas_rx];
    
    QHBoxLayout *rssi = new QHBoxLayout;
    rssi->setSpacing(8);
    
    //QVBoxLayout *specific_layout = new QVBoxLayout;
    specific_layout->addRow(new QLabel("<html><b>UE Id  </b></html>"), id_choice);
    specific_layout->addRow(new QLabel("<html><b>Position  </b></html>"), specific_position);
    specific_layout->addRow(new QLabel("<html><b>State  </b></html>"), specific_state);
    specific_layout->addRow(new QLabel("<html><b>Connected eNb  </b></html>"), specific_connected_enb);
    specific_layout->addRow(new QLabel("<html><b>Dist. to eNb</b></html>"), specific_dist);
    specific_layout->addRow(new QLabel("<html><b>Pathloss</b></html>"), specific_pathloss);
    specific_layout->addRow(new QLabel("<html><b>RNTI</b></html>"), specific_rnti);
    specific_layout->addRow(new QLabel("<html><b>RSRP</b></html>"), specific_rsrp);
    specific_layout->addRow(new QLabel("<html><b>RSRQ</b></html>"), specific_rsrq);
    
    rssi->addWidget(new QLabel("<html><b>RSSI  </b></html>"));
    
    for (int ant = 0; ant < nb_antennas_rx; ant++){
	rssi_tab[ant].setAlignment(Qt::AlignCenter);
	rssi_tab[ant].setText("-1"); // to be taken from geo (struct) 
	rssi_tab[ant].setFrameStyle( QFrame::Raised | QFrame::Box );
	rssi_tab[ant].setFixedWidth(35);
	rssi->addWidget(&rssi_tab[ant]);
    }
    
    specific_layout->addRow(rssi);
    
    
    //specific_layout->addWidget(supervised_node);  
    specific->setLayout(specific_layout);
    
    QGroupBox *cntl = new QGroupBox("Control Panel", this);
    QVBoxLayout *cntl_layout = new QVBoxLayout;
    cntl_layout->addWidget(drawConnections);
    cntl_layout->addWidget(useMap);
    cntl_layout->addWidget(used_map_frame);
    cntl_layout->addWidget(node_color_frame);
    cntl_layout->addWidget(link_color_frame);
    cntl_layout->addWidget(node_size);
    cntl->setLayout(cntl_layout);

    /* Control area */
    control_field = new QFrame(this);
    control_field->setFrameShape(QFrame::StyledPanel);
    control_field->setFrameStyle( QFrame::Sunken | QFrame::Panel );
    control_field->setLineWidth( 2 );

    QVBoxLayout *control_layout = new QVBoxLayout;
    control_layout->addWidget(generic);
    control_layout->addWidget(specific);
    control_layout->addWidget(cntl);
    control_field->setLayout(control_layout);

    /* Drawing area */
    openGL_field = new QFrame(this);
    openGL_field->setFrameStyle( QFrame::Sunken | QFrame::Panel );
    openGL_field->setLineWidth( 2 );
    openGl = new OpenGLWidget();
    openGl->setFixedSize(620,540);
    QVBoxLayout *l1 = new QVBoxLayout;
    l1->addWidget(openGl);
    openGL_field->setLayout(l1);
    
    /* Console area */
    console_field = new QFrame(this);
    output = new QTextEdit();
    output->setReadOnly(true);
    QVBoxLayout *l2 = new QVBoxLayout;
    l2->addWidget(output);
    console_field->setLayout(l2);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(openGL_field, 0, 0, 3, 3);
    layout->addWidget(control_field, 0, 3, 4, 1);
    layout->addWidget(console_field, 3, 0, 1, 3);

    this->setLayout(layout);

    communication_thread = new CommunicationThread(this);
    communication_thread->start();
}

QTextEdit* MyWindow::getConsoleField(){
    return this->output;
}

OpenGLWidget *MyWindow::getGL(){
    return this->openGl;
}

void MyWindow::importMap(){
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), uri, tr("Images (*.png *.jpeg *.jpg *.bmp *.gif)"));
    openGl->setUsedMap(fileName);
}

void MyWindow::endOfTheSimulation(){
    communication_thread->wait();  // do not exit before the thread is completed!
    delete communication_thread;
    emit exitSignal();
}

void MyWindow::setDrawConnections(int draw){
    openGl->setDrawConnections(draw);
}

void MyWindow::setUseMap(int use){
    if (!use)
      used_map->setEnabled(false);
    else
      used_map->setEnabled(true);
    openGl->setUseMap(use);
}

void MyWindow::setUsedMap(int map){
    if (map > 0)
      openGl->setUsedMap(map);
    else
      importMap();
}

void MyWindow::setNodesColor(int index){
    openGl->setNodesColor(index);
}

void MyWindow::setLinksColor(int index){
    openGl->setLinksColor(index);
}

void MyWindow::updateSize(int size){
    openGl->updateNodeSize(size);
}

void MyWindow::updateSupervNode(int id){
    supervised_id = id;
}

void MyWindow::updateSupervData(){
  
}

void MyWindow::writeToConsole(QString data, int frame){
    this->output->append(data);
    
    QString string;
    
    //updating the generic information zone -- only the frame
    string.sprintf("%d", frame);
    generic_frame->setText(string);
    
    //updating the specific information zone
    
    //Updating position
    string.sprintf("(%d,%d)",geo[supervised_id + nb_enb].x,geo[supervised_id + nb_enb].y);
    specific_position->setText(string);
    
    //Updating state
    switch (geo[supervised_id + nb_enb].state) {
      case 0:
      specific_state->setText("NOT_SYNC");
      break;
      
      case 1:
      specific_state->setText("SYNCED");
      break;
      
      case 2:
      specific_state->setText("CONNECTED");
      break;
      
      case 3:
      specific_state->setText("ATTACHED");
      break;
      
      case 4:
      specific_state->setText("DATA_COMMUNICATION");
      break;
      
    }
    
    //Updating connected eNb and distance to it
    if (geo[supervised_id + nb_enb].Neighbors) {
      int x1 = geo[geo[supervised_id + nb_enb].Neighbor[0]].x;
      int x2 = geo[supervised_id + nb_enb].x;
      int y1 = geo[geo[supervised_id + nb_enb].Neighbor[0]].y;
      int y2 = geo[supervised_id + nb_enb].y;
      float dist = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
      
      string.sprintf("%.2f", dist);
    } else string = "-1";
    
    specific_dist->setText(string);
    
    if (geo[supervised_id + nb_enb].Neighbors) {
      string.sprintf("%d", geo[supervised_id + nb_enb].Neighbor[0]);
    } else string = "-1";
    
    specific_connected_enb->setText(string);
    
    //Updating pathloss
    string.sprintf("%d", geo[supervised_id + nb_enb].Pathloss);
    specific_pathloss->setText(string);

     //Updating RNTI, RSRP and RSRQ
    string.sprintf("%d", geo[supervised_id + nb_enb].rnti);
    specific_rnti->setText(string);
  
    string.sprintf("%d", geo[supervised_id + nb_enb].RSRP);
    specific_rsrp->setText(string);
    
    string.sprintf("%d", geo[supervised_id + nb_enb].RSRQ);
    specific_rsrq->setText(string);
    
    for (int ant = 0; ant < nb_antennas_rx; ant++) {
      string.sprintf("%d", geo[supervised_id + nb_enb].RSSI[ant]);
      rssi_tab[ant].setText(string);
    }
}

MyWindow::~MyWindow(){
    if (::close (pfd[0]) == -1 ) /* we close the read desc. */
        perror( "close on read" );
    delete rssi_tab;
}
