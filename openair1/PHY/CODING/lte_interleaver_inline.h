extern unsigned int threegpplte_interleaver_output;
extern unsigned int threegpplte_interleaver_tmp;

static inline void threegpplte_interleaver_reset(void)
{
  threegpplte_interleaver_output = 0;
  threegpplte_interleaver_tmp    = 0;
}

static inline unsigned short threegpplte_interleaver(unsigned short f1,
    unsigned short f2,
    unsigned short K)
{

  threegpplte_interleaver_tmp = (threegpplte_interleaver_tmp+(f2<<1));

  threegpplte_interleaver_output = (threegpplte_interleaver_output + threegpplte_interleaver_tmp + f1 - f2)%K;

#ifdef DEBUG_TURBO_ENCODER
  printf("Interleaver output %d\n",threegpplte_interleaver_output);
#endif
  return(threegpplte_interleaver_output);
}


static inline short threegpp_interleaver_parameters(unsigned short bytes_per_codeword)
{
  if (bytes_per_codeword<=64)
    return (bytes_per_codeword-5);
  else if (bytes_per_codeword <=128)
    return (59 + ((bytes_per_codeword-64)>>1));
  else if (bytes_per_codeword <= 256)
    return (91 + ((bytes_per_codeword-128)>>2));
  else if (bytes_per_codeword <= 768)
    return (123 + ((bytes_per_codeword-256)>>3));
  else {
#ifdef DEBUG_TURBO_ENCODER
    printf("Illegal codeword size !!!\n");
#endif
    return(-1);
  }
}
