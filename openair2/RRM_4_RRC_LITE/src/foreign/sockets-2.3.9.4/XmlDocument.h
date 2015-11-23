/**
 ** \file XmlDocument.h
 ** \date  2008-02-09
 ** \author grymse@alhem.net
**/
/*
Copyright (C) 2008-2010  Anders Hedstrom

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
#ifndef _XmlDocument_H
#define _XmlDocument_H

#include "sockets-config.h"

#ifdef ENABLE_XML

#include <string>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE
{
#endif
namespace Xml
{


class XmlDocument
{
public:
  XmlDocument(const std::string& filename,const std::string& verify_ns = "",const std::string& verify_root = "");
  virtual ~XmlDocument();

  /** Document parse successful. */
  bool IsOk() {
    return m_ok;
  }

  operator xmlDocPtr();

protected:
  XmlDocument(const XmlDocument& ) {} // copy constructor

private:
  XmlDocument& operator=(const XmlDocument& ) {
    return *this;  // assignment operator
  }

  xmlDocPtr m_doc;
  bool m_ok;
};


}
#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // ENABLE_XML

#endif // _XmlDocument_H
