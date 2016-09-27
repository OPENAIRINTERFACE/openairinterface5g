/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#    ifndef _RRCMESSAGEHANDLER_H
#        define _RRCMESSAGEHANDLER_H

#        include <pthread.h>

#        include "StdoutLog.h"
#        include "SocketHandlerEp.h"
#        include "RRCUdpSocket.h"
#        include "ListenSocket.h"
#        include "Message.h"
#        include "platform.h"


class RRCMessageHandler
{
public:
  static RRCMessageHandler *Instance ();
  void            NotifyRxData (const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP);
  void            Send2Peer (Message *);
  void            Send2Peer(std::string ip_dest_strP, int port_destP, const char *in_bufferP, msg_length_t size_dataP);
  void*           ThreadLoop(void *arg);
  void            Join(bool  *quitP);
  ~RRCMessageHandler ();

private:
  RRCMessageHandler ();

  SocketHandler     *m_socket_handler;
  RRCUdpSocket      *m_socket;
  StdoutLog         *m_log;
  pthread_t          m_thread;
  bool              *m_quit;
  static RRCMessageHandler *s_instance;
};
#    endif

