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


//IDT DISTRIBUTION PARAMETERS
#define IDT_DIST GAUSSIAN
#define IDT_MIN 2
#define IDT_MAX 10
#define IDT_STD_DEV 1
#define IDT_LAMBDA 3

//TRANSPORT PROTOCOL
#define TRANS_PROTO TCP
#define IP_V IPV4

//DATA PACKET SIZE DISTRIBUTION PARAMETERS
#define PKTS_SIZE_DIST POISSON
#define PKTS_SIZE_MIN 17
#define PKTS_SIZE_MAX 1500
#define PKTS_SIZE_STD_DEV 30
#define PKTS_SIZE_LAMBDA 500

//SOCKET MODE
#define DST_PORT 1234;
#define DST_IP "127.0.0.1"
