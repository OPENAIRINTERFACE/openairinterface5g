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

/*! \file openglwidget.cpp
* \brief area devoted to draw the nodes and their connections
* \author M. Mosli
* \date 2012
* \version 0.1
* \company Eurecom
* \email: mosli@eurecom.fr
*/


#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QGLWidget>
#include <QString>
#include "qgl.h"
#include "structures.h"

class OpenGLWidget : public QGLWidget
{
  Q_OBJECT

public:
  OpenGLWidget();
  void drawGrid();
  void drawNodes();
  void loadTexture();
  void drawConnections();
  void drawSquare(int digit, int back, int w, int h, int sw, int sh);
  void drawBaseStation(int digit);
  void setDrawConnections(int draw);
  void setUseMap(int use);
  void setUsedMap(int map);
  void setUsedMap(QString map_path);
  void setLinksColor(int index);
  void setNodesColor(int index);
  void updateNodeSize(int size);
  ~OpenGLWidget();

protected:
  void paintGL();

public slots:
  void drawNewPosition();

private:
  GLuint textures[9];
  bool draw_connections;
  QImage b_station;
};

#endif // OPENGLWIDGET_H
