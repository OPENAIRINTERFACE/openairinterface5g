#include <stdio.h>
#include "RRCMessageHandler.h"


RRCMessageHandler* RRCMessageHandler::s_instance = 0;

//----------------------------------------------------------------------------
void* RRCMessageHandlerThreadLoop(void *arg)
//----------------------------------------------------------------------------
{

    return RRCMessageHandler::Instance()->ThreadLoop(arg);
}
//-----------------------------------------------------------------
RRCMessageHandler* RRCMessageHandler::Instance()
//-----------------------------------------------------------------
{
  if (RRCMessageHandler::s_instance == 0) {
        RRCMessageHandler::s_instance = new RRCMessageHandler;
  }
  return s_instance;
}
//-----------------------------------------------------------------
RRCMessageHandler::RRCMessageHandler()
//-----------------------------------------------------------------
{
  m_socket_handler = new SocketHandler();
  m_log            = new StdoutLog() ;
  m_socket_handler->RegStdLog(m_log);

  m_socket = new RRCUdpSocket(*m_socket_handler, 16384, true);
  port_t port = 33333;
  Ipv6Address ad("0::1", port);
  if (m_socket->Bind(ad, 1) == -1) {
      printf("Exiting...\n");
      exit(-1);
  } else {
      printf("Ready to receive on port %d\n",port);
  }
  m_socket_handler->Add(m_socket);

  if (pthread_create(&m_thread, NULL, RRCMessageHandlerThreadLoop, (void *) NULL) != 0) {
  	fprintf(stderr, "\nRRCMessageHandler::RRCMessageHandler() ERROR pthread_create...\n");
  } else {
      pthread_setname_np( m_thread, "RRCMsgHandler" );
  }
}
//----------------------------------------------------------------------------
void* RRCMessageHandler::ThreadLoop(void *arg)
//----------------------------------------------------------------------------
{

    m_socket_handler->Select(1,0);

    while (m_socket_handler->GetCount() && !*m_quit) {
        m_socket_handler->Select(1,0);
    }
    fprintf(stderr, "\nRRCMessageHandler::ThreadLoop Exiting...\n");
    return NULL;
}
//----------------------------------------------------------------------------
void RRCMessageHandler::Join(bool *quitP)
//----------------------------------------------------------------------------
{
  m_quit = quitP;

  pthread_join(m_thread, NULL);
  fprintf(stderr, "\nRRCMessageHandler::Join Done\n");
}

//----------------------------------------------------------------------------
void RRCMessageHandler::NotifyRxData(const char *in_bufferP,size_t size_dataP,struct sockaddr *sa_fromP,socklen_t sa_lenP)
//----------------------------------------------------------------------------
{
  Message* message;
  do {
      //message  = Message::Deserialize(in_bufferP, size_dataP, sa_fromP, sa_lenP);
      message  = Message::DeserializeRRCMessage(in_bufferP, size_dataP, sa_fromP, sa_lenP);
      if (message != NULL) {
          //fprintf(stderr, "RRCMessageHandler::notifyRxData GOT MESSAGE\n");
          message->Forward();
          delete message;
          message = NULL;
      } else {
        // TO DO
      }
  } while (message != NULL);
}
//-----------------------------------------------------------------
void RRCMessageHandler::Send2Peer(Message* messageP)
//-----------------------------------------------------------------
{
    Send2Peer(messageP->GetSrcAddress(), messageP->GetSrcPort(),  messageP->GetSerializedMessageBuffer(),  messageP->GetSerializedMessageSize());
    delete messageP;
}
//-----------------------------------------------------------------
void RRCMessageHandler::Send2Peer(std::string ip_dest_strP, int port_destP, const char *in_bufferP, msg_length_t size_dataP)
//-----------------------------------------------------------------
{
    std::string lo_bad_address("1");
    std::string lo_good_address("0::1");
    if (ip_dest_strP.compare(lo_bad_address) != 0) {
        m_socket->SendToBuf(ip_dest_strP, port_destP, in_bufferP, size_dataP, 0);
    } else {
        m_socket->SendToBuf(lo_good_address, port_destP, in_bufferP, size_dataP, 0);
    }
}
//-----------------------------------------------------------------
RRCMessageHandler::~RRCMessageHandler()
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

