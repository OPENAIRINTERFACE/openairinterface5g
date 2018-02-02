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
