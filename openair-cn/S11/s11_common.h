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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#ifndef S11_COMMON_H_
#define S11_COMMON_H_

#define S11_DEBUG(x, args...) fprintf(stdout, "[S11] [D]"x, ##args)
#define S11_INFO(x, args...)  fprintf(stdout, "[S11] [I]"x, ##args)
#define S11_WARN(x, args...)  fprintf(stdout, "[S11] [W]"x, ##args)
#define S11_ERROR(x, args...) fprintf(stderr, "[S11] [E]"x, ##args)

NwRcT s11_ie_indication_generic(uint8_t  ieType,
                                uint8_t  ieLength,
                                uint8_t  ieInstance,
                                uint8_t *ieValue,
                                void  *arg);

#endif /* S11_COMMON_H_ */
