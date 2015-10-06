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
void adc(double **r_re,
         double **r_im,
         unsigned int input_offset,
         unsigned int output_offset,
         unsigned int **output,
         unsigned int nb_rx_antennas,
         unsigned int length,
         unsigned char B)
{

  int i;
  int aa;
  double gain = (double)(1<<(B-1));
  //double gain = 1.0;

  for (i=0; i<length; i++) {
    for (aa=0; aa<nb_rx_antennas; aa++) {
      ((short *)output[aa])[((i+output_offset)<<1)]   = (short)(r_re[aa][i+input_offset]*gain);
      ((short *)output[aa])[1+((i+output_offset)<<1)] = (short)(r_im[aa][i+input_offset]*gain);

      if ((r_re[aa][i+input_offset]*gain) > 30000) {
        //("Adc outputs %d %e  %d \n",i,((short *)output[0])[((i+output_offset)<<1)], ((i+output_offset)<<1) );
      }
    }

    //printf("Adc outputs %d %e  %d \n",i,((short *)output[0])[((i+output_offset)<<1)], ((i+output_offset)<<1) );
  }
}
