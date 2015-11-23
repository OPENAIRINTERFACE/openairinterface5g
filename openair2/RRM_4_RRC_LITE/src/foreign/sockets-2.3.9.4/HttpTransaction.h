/**
 ** \file HttpTransaction.h
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
#ifndef _SOCKETS_HttpTransaction_H
#define _SOCKETS_HttpTransaction_H

#include "Utility.h"


#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE
{
#endif


class HttpTransaction
{
public:
  HttpTransaction();
  virtual ~HttpTransaction();

  /** Set / Read http header value */
  void SetHeader(const std::string& key, const std::string& value);
  void SetHeader(const std::string& key, long value);
  const std::string& Header(const std::string& key) const;

  void SetAccept(const std::string& value);
  const std::string& Accept() const;

  void SetAcceptCharset(const std::string& value);
  const std::string& AcceptCharset() const;

  void SetAcceptEncoding(const std::string& value);
  const std::string& AcceptEncoding() const;

  void SetAcceptLanguage(const std::string& value);
  const std::string& AcceptLanguage() const;

  void SetConnection(const std::string& value);
  const std::string& Connection() const;

  void SetContentType(const std::string& value);
  const std::string& ContentType() const;

  void SetContentLength(long value);
  long ContentLength() const;

  void SetHost(const std::string& value);
  const std::string& Host() const;
  const std::string HostOnly() const;

  void SetPragma(const std::string& value);
  const std::string& Pragma() const;

  void SetReferer(const std::string& value);
  const std::string& Referer() const;

  void SetUserAgent(const std::string& value);
  const std::string& UserAgent() const;

  const Utility::ncmap<std::string>& Headers() const;

  virtual void Reset();

private:
  Utility::ncmap<std::string> m_header;
  std::string m_null;

}; // end of class


#ifdef SOCKETS_NAMESPACE
} // namespace SOCKETS_NAMESPACE {
#endif

#endif // _SOCKETS_HttpTransaction_H

