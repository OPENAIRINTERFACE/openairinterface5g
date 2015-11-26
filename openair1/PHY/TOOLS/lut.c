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
double interp(double x, double *xs, double *ys, int count)
{
  int i;
  double dx, dy;

  if (x < xs[0]) {
    return 1.0; /* return minimum element */
  }

  if (x > xs[count-1]) {
    return 0.0; /* return maximum */
  }

  /* find i, such that xs[i] <= x < xs[i+1] */
  for (i = 0; i < count-1; i++) {
    if (xs[i+1] > x) {
      break;
    }
  }

  /* interpolate */
  dx = xs[i+1] - xs[i];
  dy = ys[i+1] - ys[i];
  return ys[i] + (x - xs[i]) * dy / dx;
}
