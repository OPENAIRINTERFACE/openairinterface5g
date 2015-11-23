#include <stdio.h>
#include "RRMMessageHandler.h"


RRMMessageHandler* RRMMessageHandler::s_instance = 0;

//----------------------------------------------------------------------------
void* RRMMessageHandlerThreadLoop(void *arg)
//----------------------------------------------------------------------------
{

    return RRMMessageHandler::Instance()->ThreadLoop(arg);
}
//-----------------------------------------------------------------
RRMMessageHandler* RRMMessageHandler::Instance()
//-----------------------------------------------------------------
{
  if (RRMMessageHandler::s_instance == 0) {
        RRMMessageHandler::s_instance = new RRMMessageHandler;
  }
  return s_instance;
}
//-----------------------------------------------------------------
RRMMessageHandler::RRMMessageHandler()
//-----------------------------------------------------------------
{
  m_socket_handler = new SocketHandler();
  m_log            = new StdoutLog() ;
  m_socket_handler->RegStdLog(m_log);

  m_socket = new RRMUdpSocket(*m_socket_handler, 16384, true);
  port_t port = 33334;
  Ipv6Address ad("0::1",port);
  if (m_socket->Bind(ad, 1) == -1) {
      printf("Exiting...\n");
      exit(-1);
  } else {
      printf("Ready to receive on port %d\n",port);
  }
  m_socket_handler->Add(m_socket);

  if (pthread_create(&m_thread, NULL, RRMMessageHandlerThreadLoop, (void *) NULL) != 0) {
  	fprintf(stderr, "\nRRMMessageHandler::RRMMessageHandler() ERROR pthread_create...\n");
  } else {
      pthread_setname_np( m_thread, "RRMMsgHandler" );
  }
}
//----------------------------------------------------------------------------
void* RRMMessageHandler::ThreadLoop(void *arg)
//----------------------------------------------------------------------------
{

    m_socket_handler->Select(1,0);

    while (m_socket_handler->GetCount() && !*m_quit) {
        m_socket_handler->Select(1,0);
    }
    fprintf(stderr, "\nRRMMessageHandler::ThreadLoop Exiting...\n");
    return NULL;
}
//----------------------------------------------------------------------------
void RRMMessageHandler::Join(bool *quitP)
//----------------------------------------------------------------------------
{
  m_quit = quitP;

  pthread_join(m_thread, NULL);
  fprintf(stderr, "\nRRMMessageHandler::Join Done\n");
}

//----------------------------------------------------------------------------
void RRMMessageHandler::NotifyRxData(const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP)
//----------------------------------------------------------------------------
{
  Message* message;
  do {
      fprintf(stderr, "RRMMessageHandler::notifyRxData GOT MESSAGE %d bytes\n", size_dataP);
      message  = Message::DeserializeRRMMessage(in_bufferP, size_dataP, sa_fromP, sa_lenP);
      if (message != NULL) {
          message->Forward();
          delete message;
          message = NULL;
      } else {
        // TO DO
      }
  } while (message != NULL);
}
//-----------------------------------------------------------------
void RRMMessageHandler::Send2Peer(Message* messageP)
//-----------------------------------------------------------------
{
    Send2Peer(messageP->GetSrcAddress(), messageP->GetSrcPort(),  messageP->GetSerializedMessageBuffer(),  messageP->GetSerializedMessageSize());
    delete messageP;
}
//-----------------------------------------------------------------
void RRMMessageHandler::Send2Peer(std::string ip_dest_strP, int port_destP, const char *in_bufferP, size_t size_dataP)
//-----------------------------------------------------------------
{
    m_socket->SendToBuf(ip_dest_strP, port_destP, in_bufferP, size_dataP, 0);
}
//-----------------------------------------------------------------
RRMMessageHandler::~RRMMessageHandler()
//-----------------------------------------------------------------
{
  if (m_socket_handler != 0)
  {
    delete m_socket_handler;
  }
  if (m_log != 0)
  {
    delete m_log;
  }
  // m_socket should be closed and deleted by m_socket_handler
}

