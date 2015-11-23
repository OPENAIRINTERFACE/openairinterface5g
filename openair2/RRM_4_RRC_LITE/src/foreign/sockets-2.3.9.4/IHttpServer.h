/**
 ** \file IHttpServer.h
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
#ifndef _SOCKETS_IHttpServer_H
#define _SOCKETS_IHttpServer_H

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE
{
#endif


class HttpRequest;
class HttpResponse;

class IHttpServer
{
public:
  virtual ~IHttpServer() {}

  /** Complete request has been received and parsed. Send response
      using the Respond() method. */
  virtual void OnExec(const HttpRequest& req) = 0;

  /** Send response. */
  virtual void Respond(const HttpResponse& res) = 0;

  /** Called when the body part of the response has been sent. */
  virtual void OnResponseComplete() = 0;
};




#ifdef SOCKETS_NAMESPACE
} // namespace SOCKETS_NAMESPACE {
#endif

#endif // _SOCKETS_IHttpServer_H

