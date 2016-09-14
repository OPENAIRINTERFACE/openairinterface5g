typedef struct interleaver_codebook {
  unsigned long nb_bits;
  unsigned short f1;
  unsigned short f2;
  unsigned int beg_index;
} t_interleaver_codebook;
#ifndef INCL_INTERLEAVE
extern t_interleaver_codebook f1f2mat[];
extern short il_tb[];
extern short reverse_il_tl[];
#endif
