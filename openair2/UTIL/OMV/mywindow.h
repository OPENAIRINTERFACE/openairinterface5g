/*******************************************************************************

  Eurecom OpenAirInterface 2
  Copyright(c) 1999 - 2010 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fsr/openairinterface
  Address      : Eurecom, 2229, route des crêtes, 06560 Valbonne Sophia Antipolis, France

*******************************************************************************/


/*! \file mywindow.h
* \brief manages the window and its components
* \author M. Mosli
* \date 2012
* \version 0.1
* \company Eurecom
* \email: mosli@eurecom.fr
*/

#ifndef MYWINDOW_H
#define MYWINDOW_H

#include <QPushButton>
#include <QFrame>
#include <QTextEdit>
#include <QLabel>
#include <QApplication>
#include <QComboBox>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include "openglwidget.h"

class MyWindow : public QWidget
{
  Q_OBJECT

public:
  MyWindow();
  QTextEdit* getConsoleField();
  OpenGLWidget* getGL();
  void importMap();
  ~MyWindow();

public slots:
  void writeToConsole(QString data, int frame);
  void endOfTheSimulation();
  void setDrawConnections(int draw);
  void setUseMap(int use);
  void setUsedMap(int map);
  void setNodesColor(int index);
  void setLinksColor(int index);
  void updateSize(int size);
  void updateSupervNode(int id);
  void updateSupervData();

signals:
  void exitSignal();

private:
  int pattern;
  QComboBox *used_map;
  QLabel *rssi_tab;
  QLabel *specific_position;
  QLabel *specific_state;
  QLabel *specific_dist;
  QLabel *specific_connected_enb;
  QLabel *specific_pathloss;
  QLabel *specific_rnti;
  QLabel *specific_rsrp;
  QLabel *specific_rsrq;
  QLabel *generic_frame;
  QLabel *simulation_data;
  QFrame *control_field;
  QFrame *openGL_field;
  OpenGLWidget *openGl;
  QFrame *console_field;
  QTextEdit *output;
};

#endif // MYWINDOW_H
