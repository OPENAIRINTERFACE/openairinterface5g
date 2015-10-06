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
unsigned char log2_approx(unsigned int x)
{

  int i;
  unsigned char l2;

  l2=0;

  for (i=0; i<31; i++)
    if ((x&(1<<i)) != 0)
      l2 = i+1;

  //l2 = i;

  //  printf("log2_approx = %d\n",l2);
  return(l2);
}

unsigned char log2_approx64(unsigned long long int x)
{

  int i;
  unsigned char l2;

  l2=0;

  for (i=0; i<63; i++)
    if ((x&(((long long int) 1)<<i)) != 0)
      //     l2 = i+1;
      l2 = i;

  //  printf("log2_approx = %d\n",l2);
  return(l2);
}

