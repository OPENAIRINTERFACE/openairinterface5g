#ifndef _RRMUDPSOCKET_H
#    define _RRMUDPSOCKET_H

#    include "UdpSocket.h"


class RRMUdpSocket : public UdpSocket
{
public:
  RRMUdpSocket(ISocketHandler&,int ibufsz, bool ipv6);

  void OnRawData(const char *,size_t,struct sockaddr *,socklen_t);
};


#endif // _RRMUDPSOCKET_H
