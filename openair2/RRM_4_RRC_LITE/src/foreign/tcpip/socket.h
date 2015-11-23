/************************************************************************
 ** This file is part of the network simulator Shawn.                  **
 ** Copyright (C) 2004-2007 by the SwarmNet (www.swarmnet.de) project  **
 ** Shawn is free software; you can redistribute it and/or modify it   **
 ** under the terms of the BSD License. Refer to the shawn-licence.txt **
 ** file in the root of the Shawn source tree for further details.     **
 ************************************************************************/

#ifndef __SHAWN_APPS_TCPIP_SOCKET_H
#define __SHAWN_APPS_TCPIP_SOCKET_H

#ifdef SHAWN
#include <shawn_config.h>
#include "_apps_enable_cmake.h"
#ifdef ENABLE_TCPIP
#define BUILD_TCPIP
#endif
#else
#define BUILD_TCPIP
#endif


#ifdef BUILD_TCPIP

// Get Storage
#ifdef SHAWN
#include <apps/tcpip/storage.h>
#else
#include "storage.h"
#endif

#ifdef SHAWN
namespace shawn
{
class SimulationController;
}

// Dummy function is called when Shawn Simulation starts. Does nothing up to now.
extern "C" void init_tcpip( shawn::SimulationController& );
#endif

// Disable exception handling warnings
#ifdef WIN32
#pragma warning( disable : 4290 )
#endif

#include <string>
#include <map>
#include <vector>
#include <list>
#include <deque>
#include <iostream>


struct in_addr;

namespace tcpip
{

class SocketException: public std::exception
{
private:
  std::string what_;
public:
  SocketException( std::string what ) throw() {
    what_ = what;
    //std::cerr << "tcpip::SocketException: " << what << std::endl << std::flush;
  }

  virtual const char* what() const throw() {
    return what_.c_str();
  }

  ~SocketException() throw() {}
};

class Socket
{
  friend class Response;
public:
  /// Constructor that prepare to connect to host:port
  Socket(std::string host, int port);

  /// Constructor that prepare for accepting a connection on given port
  Socket(int port);

  /// Destructor
  ~Socket();

  /// Connects to host_:port_
  void connect() throw( SocketException );

  /// Wait for a incoming connection to port_
  void accept() throw( SocketException );

  void send( const std::vector<unsigned char> ) throw( SocketException );
  void sendExact( const Storage & ) throw( SocketException );
  std::vector<unsigned char> receive( int bufSize = 2048 ) throw( SocketException );
  bool receiveExact( Storage &) throw( SocketException );
  void close();
  int port();
  void set_blocking(bool) throw( SocketException );
  bool is_blocking() throw();
  bool has_client_connection() const;

  // If verbose, each send and received data is written to stderr
  bool verbose() {
    return verbose_;
  }
  void set_verbose(bool newVerbose) {
    verbose_ = newVerbose;
  }

private:
  void init();
  void BailOnSocketError( std::string ) const throw( SocketException );
#ifdef WIN32
  std::string GetWinsockErrorString(int err) const;
#endif
  bool atoaddr(std::string, struct in_addr& addr);
  bool datawaiting(int sock) const throw();

  std::string host_;
  int port_;
  int socket_;
  int server_socket_;
  bool blocking_;

  bool verbose_;
#ifdef WIN32
  static bool init_windows_sockets_;
  static bool windows_sockets_initialized_;
  static int instance_count_;
#endif
};

} // namespace tcpip

#endif // BUILD_TCPIP

#endif

/*-----------------------------------------------------------------------
* Source  $Source: $
* Version $Revision: 197 $
* Date    $Date: 2008-04-29 17:40:51 +0200 (Tue, 29 Apr 2008) $
*-----------------------------------------------------------------------
* $Log:$
*-----------------------------------------------------------------------*/
