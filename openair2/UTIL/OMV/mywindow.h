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
