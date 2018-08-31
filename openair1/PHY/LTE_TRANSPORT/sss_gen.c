#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>

int16_t *d0_sss;
int16_t *d5_sss;

#define MyAssert(x) { if(!(x)) { printf("Error in table intialization: %s:%d\n",__FILE__,__LINE__); exit(1);}}
#define gen(table, formula) {     \
    int x[31]= {0};       \
    x[4]=1;         \
    for(int i=0; i<26; i++)     \
      x[i+5]=formula;       \
    for (int i=0; i<31; i++)      \
      table[i]=1-2*x[i];      \
  }

#define mod31(a) (a)%31
#define mod2(a)  (a)%2
#define mod8(a)  (a)%8
void init_sss(void) {
  MyAssert(0==posix_memalign((void **)&d0_sss, 16,504*31*2*sizeof(*d0_sss)));
  MyAssert(0==posix_memalign((void **)&d5_sss, 16,504*31*2*sizeof(*d5_sss)));
  int s[31];
  gen(s, mod2(x[i+2]+x[i]));
  int z[31];
  gen(z, mod2(x[i+4]+x[i+2]+x[i+1]+x[i]));
  int c[31];
  gen(c, mod2(x[i+3]+x[i]));

  for (int Nid2=0; Nid2<3; Nid2++) {
    for (int Nid1=0; Nid1<168; Nid1++) {
      int qprime = Nid1/30;
      int q = (Nid1+(qprime*(qprime+1))/2)/30;
      int mprime = Nid1 + q*(q+1)/2;
      int m0 = mprime%31;
      int m1 = (m0+mprime/31+1)%31;
      int rowIndex=(Nid2+3*Nid1)*31*2;

      for (int i=0; i<31; i++) {
        d0_sss[rowIndex+i*2]=   s[mod31(i+m0)] * c[mod31(i+Nid2)];
        d5_sss[rowIndex+i*2]=   s[mod31(i+m1)] * c[mod31(i+Nid2)];
        d0_sss[rowIndex+i*2+1]= s[mod31(i+m1)] * c[mod31(i+Nid2+3)] * z[mod31(i+mod8(m0))];
        d5_sss[rowIndex+i*2+1]= s[mod31(i+m0)] * c[mod31(i+Nid2+3)] * z[mod31(i+mod8(m1))];
      }
    }
  }
}

#ifdef SSS_TABLES_TEST
void main () {
  printf("int16_t d0_sss[504*62] = {");

  for (int i=0; i<504*62; i++)
    printf("%d,\n",d0_sss[i]);

  printf("};\n\n");
  printf("int16_t d5_sss[504*62] = {");

  for (int i=0; i<504*62; i++)
    printf("%d,\n",d5_sss[i]);

  printf("};\n\n");
}
#endif
