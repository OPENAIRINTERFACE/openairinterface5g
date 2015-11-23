/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

// ===========================================================================
#include <stdio.h>
#include <signal.h>
// ===========================================================================
#include "RRCMessageHandler.h"
// ===========================================================================
static	bool quit = false;
// ===========================================================================
void sigint(int)
// ===========================================================================
{
  quit = true;
}
// ===========================================================================
int main(int argc,char *argv[])
// ===========================================================================
{

  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, sigint);

  // start RRC server
  RRCMessageHandler::Instance();
  RRCMessageHandler::Instance()->Join(&quit);
  fprintf(stderr, "\nRRM LTE Exiting...\n");
  return 0;
}


