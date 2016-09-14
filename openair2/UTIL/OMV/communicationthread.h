/*! \file communicationthread.h
* \brief this thread is to process the communication between the simulator and the visualisor
* \author M. Mosli
* \date 2012
* \version 0.1
* \company Eurecom
* \email: mosli@eurecom.fr
*/

#ifndef COMMUNICATIONTHREAD_H
#define COMMUNICATIONTHREAD_H

#include <QThread>
#include <QDebug>
#include <QTextEdit>
#include "mywindow.h"
#include "structures.h"

class CommunicationThread : public QThread
{
  Q_OBJECT

public:
  CommunicationThread(MyWindow* window);

signals:
  void newData(QString data, int frame);
  void newPosition();
  void endOfTheSimulation();

private:
  void run();
  MyWindow* window;
};

#endif // COMMUNICATIONTHREAD_H
