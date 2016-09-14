#include "PHY/sse_intrin.h"

void Zero_Buffer(void *buf,unsigned int length)
{
  // zeroes the mmx_t buffer 'buf' starting from buf[0] to buf[length-1] in bytes
  int i;
  register __m64 mm0;
  __m64 *mbuf = (__m64 *)buf;

  //  length>>=3;                    // put length in quadwords
  mm0 = _m_pxor(mm0,mm0);         // clear the register

  for(i=0; i<length>>3; i++)     // for each i
    mbuf[i] = mm0;                // put 0 in buf[i]

  _mm_empty();
}

void mmxcopy(void *dest,void *src,int size)
{

  // copy size bytes from src to dest
  register int i;
  register __m64 mm0;
  __m64 *mmsrc = (__m64 *)src, *mmdest= (__m64 *)dest;



  for (i=0; i<size>>3; i++) {
    mm0 = mmsrc[i];
    mmdest[i] = mm0;
  }

  _mm_empty();
}

void Zero_Buffer_nommx(void *buf,unsigned int length)
{

  int i;

  for (i=0; i<length>>2; i++)
    ((int *)buf)[i] = 0;

}

