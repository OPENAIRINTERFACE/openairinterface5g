/**
 ** \file HttpBaseSocket.h
 ** \date  2007-10-05
 ** \author grymse@alhem.net
**/
/*
Copyright (C) 2007-2010  Anders Hedstrom

This library is made available under the terms of the GNU GPL, with
the additional exemption that compiling, linking, and/or using OpenSSL
is allowed.

If you would like to use this library in a closed-source application,
a separate license agreement is available. For information about
the closed-source license agreement for the C++ sockets library,
please visit http://www.alhem.net/Sockets/license.html and/or
email license@alhem.net.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef _HttpBaseSocket_H
#define _HttpBaseSocket_H

#include "HTTPSocket.h"
#include "HttpRequest.h"
#include "IHttpServer.h"
#include "HttpResponse.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE
{
#endif


class HttpResponse;

class HttpBaseSocket : public HTTPSocket, public IHttpServer
{
public:
  HttpBaseSocket(ISocketHandler& h);
  ~HttpBaseSocket();

  void OnFirst();
  void OnHeader(const std::string& key,const std::string& value);
  void OnHeaderComplete();
  void OnData(const char *,size_t);

  // implements IHttpServer::Respond
  void Respond(const HttpResponse& res);

  void OnTransferLimit();

protected:
  HttpBaseSocket(const HttpBaseSocket& s) : HTTPSocket(s) {} // copy constructor
  //
  HttpRequest m_req;
  HttpResponse m_res;
  void Reset();

private:
  HttpBaseSocket& operator=(const HttpBaseSocket& ) {
    return *this;  // assignment operator
  }
  void Execute();
  //
  size_t m_body_size_left;
  bool m_b_keepalive;
};




#ifdef SOCKETS_NAMESPACE
} // namespace SOCKETS_NAMESPACE {
#endif

#endif // _HttpBaseSocket_H

