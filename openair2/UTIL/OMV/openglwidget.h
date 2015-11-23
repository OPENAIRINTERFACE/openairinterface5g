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
