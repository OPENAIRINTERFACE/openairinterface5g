#include <stdio.h>

void print(int n)
{
  int i;
  printf("#define T%d(t", n);
  for(i=0; i<(n-1)/2; i++) printf(",t%d,x%d", i, i);
  printf(") \\\n");
  printf("  do { \\\n");
  printf("    if (T_ACTIVE(t)) { \\\n");
  printf("      T_LOCAL_DATA \\\n");
  printf("      T_HEADER(t); \\\n");
  for(i=0; i<(n-1)/2; i++) printf("      T_PUT_##t%d(%d, x%d); \\\n", i, i+2, i);
  printf("      T_COMMIT(); \\\n");
  printf("    } \\\n");
  printf("  } while (0)\n");
  printf("\n");
}

int main(void)
{
  int i;
  for (i = 11; i <= 33; i+=2) print(i);
  return 0;
}
