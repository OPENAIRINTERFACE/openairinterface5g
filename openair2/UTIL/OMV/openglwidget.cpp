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

#include "openglwidget.h"
#include <GL/glu.h>
#include <stdio.h>

extern struct Geo geo[NUMBER_OF_eNB_MAX+NUMBER_OF_UE_MAX];
extern int x_area;
extern int y_area;
extern int z_area;
extern int node_number;
extern int nb_enb;
int use_map = 1;
int node_color = 0;
int link_color = 0;
int w = 10, h = 16, sw = 24, sh = 22;
QString usedMap;

void drawMeter() {
  glBegin(GL_LINES);
    glVertex2d(-3,0);
    glVertex2d(3,0);
    glVertex2d(-3,0);
    glVertex2d(-3,-5);
    glVertex2d(0,0);	
    glVertex2d(0,-5);
    glVertex2d(3,0);
    glVertex2d(3,-5);
  glEnd();
}

OpenGLWidget::OpenGLWidget()
{
    geo[0].x = -1;
    draw_connections = true;  
    QString uri;
    QString image;
    uri.sprintf("%s/UTIL/OMV/",getenv("OPENAIR2_DIR"));
  
    image = "mus.png";
    usedMap = uri + image;
}

void OpenGLWidget::paintGL()
{

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluOrtho2D(0,620,0,540); 
  
  glColor3d(0,0,0);
  
  if (use_map)
    loadTexture();

  glTranslated(50,20,0);
  drawSquare(0, 0, 6, 10, 24, 24);
  
  glTranslated(0,500,0);
  drawMeter();
  glTranslated(-10,0,0);
  drawSquare(y_area % 10, 0, 6, 10, 24, 24);
  glTranslated(10,0,0);
  
  if (y_area >= 10) {
    glTranslated(-20,0,0);
    drawSquare((y_area % 100) / 10, 0, 6, 10, 24, 24);
    glTranslated(20,0,0);
  }
  if (y_area >= 100) {
    glTranslated(-30,0,0);
    drawSquare((y_area % 1000) / 100, 0, 6, 10, 24, 24);
    glTranslated(30,0,0);
  }
  if (y_area >= 1000) {
    glTranslated(-40,0,0);
    drawSquare(y_area / 1000, 0, 6, 10, 24, 24);
    glTranslated(40,0,0);
  }
  
  glTranslated(561,-500,0);
  drawMeter();
  glTranslated(-10,0,0);
  drawSquare(x_area % 10, 0, 6, 10, 24, 24);
  glTranslated(10,0,0);
  
  if (x_area >= 10) {
    glTranslated(-20,0,0);
    drawSquare((x_area % 100) / 10, 0, 6, 10, 24, 24);
    glTranslated(20,0,0);
  }
  if (x_area >= 100) {
    glTranslated(-30,0,0);
    drawSquare((x_area % 1000) / 100, 0, 6, 10, 24, 24);
    glTranslated(30,0,0);
  }
  if (x_area >= 1000) {
    glTranslated(-40,0,0);
    drawSquare(x_area / 1000, 0, 6, 10, 24, 24);
    glTranslated(40,0,0);
  }

  //glTranslated(-1 * (x_area >= 1000 ? 601 : (x_area >= 100 ? 611 : (x_area >= 10 ? 621 : 631))),0,0);
  glTranslated(-552,0,0);
  
  drawGrid();

  if (draw_connections)
    drawConnections();

  drawNodes();
}

void OpenGLWidget::loadTexture(){
    QImage b;
    glColor3d(255,255,255);
    
    glEnable(GL_TEXTURE_2D);

    textures[0] = bindTexture(QPixmap(usedMap), GL_TEXTURE_2D);

    glBegin(GL_QUADS);
      glTexCoord2d(0,1);  glVertex2d(58,520);
      glTexCoord2d(0,0);  glVertex2d(58,20);
      glTexCoord2d(1,0);  glVertex2d(558,20);
      glTexCoord2d(1,1);  glVertex2d(558,520);
    glEnd();
    glDisable(GL_TEXTURE_2D);

}



void OpenGLWidget::drawNewPosition(){
    updateGL();
}

void OpenGLWidget::setDrawConnections(int draw){
    this->draw_connections = draw;
    updateGL();
}

void OpenGLWidget::setUseMap(int use){
    use_map = use;
    updateGL();
}

void OpenGLWidget::setUsedMap(QString map_path){
  usedMap = map_path;
}

void OpenGLWidget::setUsedMap(int map){
    QString uri;
    QString image;
    uri.sprintf("%s/UTIL/OMV/",getenv("OPENAIR2_DIR"));
  
    switch (map){
      case 1:
      image = "mus.png";
      usedMap = uri + image;
      break;
      
      case 2:
      image = "new.jpg";
      usedMap = uri + image;
      break;
      
      case 3:
      image = "new2.jpg";
      usedMap = uri + image;
      break;
      
      case 4:
      image = "white.png";
      usedMap = uri + image;
      break;
      
      case 5:
      image = "red.png";
      usedMap = uri + image;
      break;
      
      case 6:
      image = "green.png";
      usedMap = uri + image;
      break;
      
      case 7:
      image = "blue.png";
      usedMap = uri + image;
      break;
    }
}

void OpenGLWidget::setNodesColor(int index){
    node_color = index;
    updateGL();
}

void OpenGLWidget::setLinksColor(int index){
    link_color = index;
    updateGL();
}

void OpenGLWidget::updateNodeSize(int size){
    switch (size) {
      case 1:
      w = 8;
      h = 12;
      sw = 20;
      sh = 18;
      break;
      
      case 2:
      w = 10;
      h = 16;
      sw = 24;
      sh = 22;
      break;
      
      case 3:
      w = 12;
      h = 20;
      sw = 28;
      sh = 26;
      break;
      
    }
    updateGL();
}

OpenGLWidget::~OpenGLWidget(){
    glDeleteTextures(1, textures);
}

void OpenGLWidget::drawConnections(){

    for (int i=0; i<node_number; i++){
        for (int j=i+1; j<node_number; j++){
            int k=0;

            while((geo[i].Neighbor[k]!=j)&&(k<geo[i].Neighbors)){
                k++;
            }

            if(k < geo[i].Neighbors){
	      	//glColor3d(255,255,255);
	      	switch (link_color) {
		  case 1:
		  glColor3d(255,0,0);
		  break;
      
		  case 2:
		  glColor3d(0,0,255);
		  break;
      
		  case 3:
		  glColor3d(0,255,0);
		  break;
		  
		  case 0:
		  glColor3d(255,255,255);
		  break;
		}
           
                //choose it according to the number of displayed nodes
                glLineWidth(0.7);
                glBegin(GL_LINES);
		    if (geo[i].node_type == 0) 
		      glVertex2d((int)(((float)geo[i].x/(float)x_area)*500),(int)(((float)(geo[i].y + 2 * sh)/(float)y_area)*500));
		    else
		      glVertex2d((int)(((float)geo[i].x/(float)x_area)*500),(int)(((float)geo[i].y/(float)y_area)*500));
		    if (geo[j].node_type == 0)
		      glVertex2d((int)(((float)geo[j].x/(float)x_area)*500),(int)(((float)(geo[j].y + 2 * sh)/(float)y_area)*500));
		    else
		      glVertex2d((int)(((float)geo[j].x/(float)x_area)*500),(int)(((float)geo[j].y/(float)y_area)*500));
                glEnd();
            }
        }
    }


}

void OpenGLWidget::drawNodes(){

    if (geo[0].x != -1){

        glTranslated((int)(((float)geo[0].x/(float)x_area)*500), (int)(((float)geo[0].y/(float)y_area)*500),0);
        //gluSphere(params,50,25,25);
	
	if (geo[0].node_type == 0){
	    glColor3d(0,255,0);
	    drawBaseStation(0);
	}else{
	    glColor3d(0,255,100);	
	    drawSquare(0, 1, w, h, sw, sh);
	}
	
        for (int i=1; i<node_number; i++){

	    glTranslated((int)(((float)geo[i].x/(float)x_area)*500) - (int)(((float)geo[i-1].x/(float)x_area)*500),
			(int)(((float)geo[i].y/(float)y_area)*500) - (int)(((float)geo[i-1].y/(float)y_area)*500),0);

	    if (geo[i].node_type == 0){
	        glColor3d(0,255,0);
	        drawBaseStation(i);
	    }else{
	        glColor3d(0,255,100);	
	        drawSquare(i - nb_enb, 1, w, h, sw, sh);
	    }

        }

        glTranslated(-(int)(((float)geo[node_number - 1].x/(float)x_area)*500),-(int)(((float)geo[node_number - 1].y/y_area)*500),0);
    }
}

void OpenGLWidget::drawGrid(){
    glColor3d(0,0,255);
    glLineWidth(1.0);

    glBegin(GL_LINES);

    /* Lines that are parallel to (Ox) */
    for (int i=0; i <= y_area; i+= y_area){
        glVertex2d(0,(int)(((float)i/(float)y_area)*500));
        glVertex2d(500,(int)(((float)i/(float)y_area)*500));
    }

     /* Lines that are parallel to (Oy) */
     for (int i=0; i <= x_area; i+= x_area){
        glVertex2d((int)(((float)i/(float)x_area)*500),0);
        glVertex2d((int)(((float)i/(float)x_area)*500),500);
     }

    glEnd();
}

void OpenGLWidget::drawSquare(int digit, int back, int w, int h, int sw, int sh){
  
    glColor3d(0,255,100);
   
    if (back == 1)
      if (digit < 10 && digit >= 0) {
	glBegin(GL_QUADS);
	    glVertex2d(-sw/2,-sh/2);
	    glVertex2d(-sw/2, sh/2);
	    glVertex2d( sw/2, sh/2);
	    glVertex2d( sw/2,-sh/2);
	glEnd();
      } else {
	glBegin(GL_QUADS);
	    glVertex2d(-(sw*2)/3,-sh/2);
	    glVertex2d(-(sw*2)/3, sh/2);
	    glVertex2d( (sw*2)/3, sh/2);
	    glVertex2d( (sw*2)/3,-sh/2);
	glEnd();
      }

    //draw the digit
    
    //set the right color
    
    if (back) 
      switch (node_color) {
	case 0:
	glColor3d(255,0,0);
	break;
      
	case 1:
	glColor3d(0,0,255);
	break;
      
	case 2:
	glColor3d(0,255,0);
	break;
	
	case 3:
	glColor3d(255,255,255);
	break;
      }
     else
       glColor3d(255,0,0);
    
     
    if (digit < 10 && digit >= 0) {
      glBegin(GL_LINES);
      switch (digit){
	 case 0:
	  glVertex2d(-w/2,h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(-w/2,h/2);
	  break;
	
	  case 1:
	  glVertex2d(w/2,h/2);
	  glVertex2d(w/2,-h/2);
	  break;

	  case 2:
	  glVertex2d(-w/2,h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(w/2,0);	
	  glVertex2d(-w/2,0);
	  glVertex2d(w/2,0);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(-w/2,0);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  break;

	  case 3:
	  glVertex2d(w/2,h/2);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(-w/2,h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(w/2,0);	
	  glVertex2d(-w/2,0);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  break;

	  case 4:
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,h/2);
	  glVertex2d(-w/2,0);
	  glVertex2d(-w/2,0);
	  glVertex2d(w/2,0);
	  break;

	  case 5:
	  glVertex2d(-w/2,h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,h/2);
	  glVertex2d(-w/2,0);	
	  glVertex2d(-w/2,0);
	  glVertex2d(w/2,0);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,0);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  break;

	  case 6:
	  glVertex2d(-w/2,h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,h/2);
	  glVertex2d(-w/2,-h/2);	
	  glVertex2d(-w/2,0);
	  glVertex2d(w/2,0);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,0);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  break;

	  case 7:
	  glVertex2d(w/2,h/2);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(-w/2,h/2);
	  glVertex2d(w/2,h/2);
	  break;
	  
	  case 8:
	  glVertex2d(-w/2,0);
	  glVertex2d(w/2,0);
	  glVertex2d(-w/2,h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,0);
	  glVertex2d(-w/2,h/2);
	  break;
	  
	  case 9:
	  glVertex2d(-w/2,0);
	  glVertex2d(w/2,0);
	  glVertex2d(-w/2,h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,-h/2);
	  glVertex2d(w/2,h/2);
	  glVertex2d(-w/2,-h/2);
	  glVertex2d(-w/2,h/2);
	  break;
      }
      glEnd();
      
    } else {
      
      glTranslated(w - 2,0,0);
      drawSquare(digit % 10, 2, w, h, sw, sh);
      glTranslated(-2 * w + 4 ,0,0);
      drawSquare(digit / 10, 2, w, h, sw, sh);
      glTranslated(w - 2,0,0);
    }
}

void OpenGLWidget::drawBaseStation(int digit){
   /* GLUquadric* params;
    params = gluNewQuadric();
    gluQuadricDrawStyle(params,GLU_FILL);
    
    gluDisk(params,0,sw/2,20,1);
    gluDeleteQuadric(params);*/
    
    glLineWidth(2.0);
    glBegin(GL_TRIANGLES);
	
	glVertex2d(0,2*sh);
	glVertex2d(-sw*2/3,-sh/2);
	glVertex2d(sw*2/3,-sh/2);
	
    glEnd();
    drawSquare(digit, 2, w, h, sw, sh);

}
