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

#include <iostream>
#include "TcpClient.h"

void printUsage()
{
  cout<<"usage: EthernetRawCommand <server-ip> [scpi-command]"<<endl;
}


int main( int argc, char *argv[] )
{
  int errorCode         = 0; //no error

  bool useSingleCommand = false;
  string singleCommand  = "";
  string hostname       = "";
  int    port           = 5025;

  string input          = "";
  TcpClient client;


  switch( argc )
  {
    case 3:
      useSingleCommand = true;
      singleCommand    = argv[2];
    case 2:
      hostname         = argv[1];
      break;
    default:
        printUsage();
        return(-1);
  }

  try
  {
    client.connectToServer( hostname, port );

    bool terminate = false;

    while( !terminate )
    {
      char buffer[1024];

      if( useSingleCommand )
      {
        input =  singleCommand; //send string
      }
      else
      {
        cin.getline( buffer, 1024 );
        input = buffer;

        if( input == "end" )
        {
          terminate = true;
        }

      }

      if( !terminate)
      {
        client.transmit( input ); //send string

        int qPos = input.find( "?", 0 );
        //receive string only when needed
        if( qPos > 0 )
        {
          string rcStr = "";
          client.receive( rcStr );
          cout << rcStr << endl;
        }
      }

      if( useSingleCommand )
      {
        terminate = true;
      }
    }

  }catch( const string errorString )
  {
    cout<<errorString<<endl;
  }

  client.disconnect( );

  return errorCode;
}
