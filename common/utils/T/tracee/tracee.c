#include "../T.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
  int frame = 0;
  T_connect_to_tracer("127.0.0.1", 2020);
  while (1) {
    getchar();
    T(T_ENB_PHY_PUCCH_1AB_IQ, T_INT(0), T_INT(0), T_INT(frame), T_INT(0), T_INT(0), T_INT(0));
    frame++;
  }
  return 0;
}
