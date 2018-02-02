/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <string>

//defines structs for socket handling
#include <netinet/in.h>


using namespace std;

typedef struct sockaddr_in SockAddrStruct;
typedef struct hostent     HostInfoStruct;

class TcpClient
{
public:
  TcpClient();
  ~TcpClient();

  void connectToServer( string &hostname, int port );
  void disconnect( );
  void transmit( string &txString );
  void receive( string &rxString );

  string getCurrentHostName( ) const;
  int    getCurrentPort( ) const;

private:
  string           currentHostName;
  int              currentPort;
  int              currentSocketDescr;
  SockAddrStruct   serverAddress;
  HostInfoStruct * currentHostInfo;
  bool             clientIsConnected;

  int              receiveBufferSize;
};
