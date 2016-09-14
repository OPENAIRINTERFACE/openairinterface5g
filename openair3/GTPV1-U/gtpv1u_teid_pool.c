/*! \file gtpv1u_task.c
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/
#include <stdlib.h>
#include <stdint.h>

#include "gtpv1u.h"

//#define GTPV1U_LINEAR_TEID_ALLOCATION 1

#ifdef GTPV1U_LINEAR_TEID_ALLOCATION
static uint32_t g_gtpv1u_teid = 0;
#endif

uint32_t gtpv1u_new_teid(void)
{
#ifdef GTPV1U_LINEAR_TEID_ALLOCATION
  g_gtpv1u_teid = g_gtpv1u_teid + 1;
  return g_gtpv1u_teid;
#else
  return random() + random() % (RAND_MAX - 1) + 1;
#endif
}
