#ifndef _RRMMESSAGEHANDLER_H
#    define _RRMMESSAGEHANDLER_H

#    include <pthread.h>

#    include "StdoutLog.h"
#    include "SocketHandlerEp.h"
#    include "RRMUdpSocket.h"
#    include "ListenSocket.h"
#    include "Message.h"
#    include "platform.h"


class RRMMessageHandler
{
public:
  static RRMMessageHandler *Instance ();
  void            NotifyRxData (const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP);
  void            Send2Peer (Message *);
  void            Send2Peer(std::string ip_dest_strP, int port_destP, const char *in_bufferP, size_t size_dataP);
  void*           ThreadLoop(void *arg);
  void            Join(bool  *quitP);
  ~RRMMessageHandler ();

private:
  RRMMessageHandler ();

  SocketHandler     *m_socket_handler;
  RRMUdpSocket      *m_socket;
  StdoutLog         *m_log;
  pthread_t          m_thread;
  bool              *m_quit;
  static RRMMessageHandler *s_instance;
};
#    endif

