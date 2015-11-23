#ifndef _RRCUDPSOCKET_H
#define _RRCUDPSOCKET_H

#include "UdpSocket.h"


class RRCUdpSocket : public UdpSocket
{
public:
  RRCUdpSocket(ISocketHandler&,int ibufsz, bool ipv6);

  void OnRawData(const char *,size_t,struct sockaddr *,socklen_t);
};


#endif // _RRCUDPSOCKET_H
