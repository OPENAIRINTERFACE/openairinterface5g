/**
 ** \file SSLInitializer.h
 ** \date  2007-04-30
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
#ifndef __SOCKETS_SSLInitializer_H
#define __SOCKETS_SSLInitializer_H
#include "sockets-config.h"
#ifdef HAVE_OPENSSL

#ifdef _WIN32
#include <winsock2.h>
#endif
#include <openssl/ssl.h>
#include <string>
#include <map>
#include "Mutex.h"


#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE
{
#endif


class SSLInitializer
{
public:
  /**
    init openssl
    bio_err
    create random file
  */
  SSLInitializer();

  /**
    remove random file
  */
  ~SSLInitializer();

  void DeleteRandFile();

  /** SSL; mutex locking function callback. */
  static  void SSL_locking_function(int mode, int n, const char *file, int line);

  /** Return thread id. */
  static  unsigned long SSL_id_function();

  BIO *bio_err;

private:
  std::string m_rand_file;
  long m_rand_size;
  static  std::map<int, IMutex *> *m_mmap;
  static  Mutex *m_mmap_mutex;

  static  std::map<int, IMutex *>& MMap();
  static  Mutex& MMapMutex();

};




#ifdef SOCKETS_NAMESPACE
} // namespace SOCKETS_NAMESPACE {
#endif
#endif // HAVE_OPENSSL
#endif // __SOCKETS_SSLInitializer_H

