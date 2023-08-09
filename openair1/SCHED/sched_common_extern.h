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

/*!\brief SCHED external variables */

#ifndef __SCHED_EXTERN_H__
#define __SCHED_EXTERN_H__

#include "sched_eNB.h"

// This is the formula from Section 5.1.1.1 in 36.213 100*10*log10((2^(MPR*Ks)-1)), where MPR is in the range [0,6] and Ks=1.25
static const int16_t hundred_times_delta_TF[100] = {-32768, -1268, -956, -768, -631, -523, -431, -352, -282, -219, -161, -107, -57,  -9,   36,   79,   120,  159,  197,  234,
                                                    269,    304,   337,  370,  402,  434,  465,  495,  525,  555,  583,  612,  640,  668,  696,  723,  750,  777,  803,  829,
                                                    856,    881,   907,  933,  958,  983,  1008, 1033, 1058, 1083, 1108, 1132, 1157, 1181, 1205, 1229, 1254, 1278, 1302, 1325,
                                                    1349,   1373,  1397, 1421, 1444, 1468, 1491, 1515, 1538, 1562, 1585, 1609, 1632, 1655, 1679, 1702, 1725, 1748, 1772, 1795,
                                                    1818,   1841,  1864, 1887, 1910, 1933, 1956, 1980, 2003, 2026, 2049, 2072, 2095, 2118, 2141, 2164, 2186, 2209, 2232, 2255};
static const uint16_t hundred_times_log10_NPRB[100] = {0,    301,  477,  602,  698,  778,  845,  903,  954,  1000, 1041, 1079, 1113, 1146, 1176, 1204, 1230, 1255, 1278, 1301,
                                                       1322, 1342, 1361, 1380, 1397, 1414, 1431, 1447, 1462, 1477, 1491, 1505, 1518, 1531, 1544, 1556, 1568, 1579, 1591, 1602,
                                                       1612, 1623, 1633, 1643, 1653, 1662, 1672, 1681, 1690, 1698, 1707, 1716, 1724, 1732, 1740, 1748, 1755, 1763, 1770, 1778,
                                                       1785, 1792, 1799, 1806, 1812, 1819, 1826, 1832, 1838, 1845, 1851, 1857, 1863, 1869, 1875, 1880, 1886, 1892, 1897, 1903,
                                                       1908, 1913, 1919, 1924, 1929, 1934, 1939, 1944, 1949, 1954, 1959, 1963, 1968, 1973, 1977, 1982, 1986, 1991, 1995, 2000};

void kill_fep_thread(RU_t *ru);

#endif /*__SCHED_EXTERN_H__ */
