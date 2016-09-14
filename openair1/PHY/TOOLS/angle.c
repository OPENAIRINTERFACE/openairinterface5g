/******************************
 * file: angle.c
 * purpose: compute the angle of a 8 bit complex number
 * author: florian.kaltenberger@eurecom.fr
 * date: 22.9.2009
 *******************************/

#include "costable.h"
#include "defs.h"

unsigned int angle(struct complex16 perror)
{
  int a;

  // a = atan(perror.i/perror.r);

  //since perror is supposed to be on the unit circle, we can also compute a by
  if (perror.i>=0) {
    if (perror.r>=0)
      a = acostable[min(perror.r,255)];
    else
      a = 32768-acostable[min(-perror.r,255)];

    //a = asin(perror.i);
  } else {
    if (perror.r>=0)
      a = 65536-acostable[min(perror.r,255)];
    else
      a = 32768+acostable[min(-perror.r,255)];

    //a = 2*PI-asin(perror.i);
  }

  return a;

}
