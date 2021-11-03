//int nr_generate_prs(){}

int nr_generate_pbch_dmrs(uint32_t *gold_pbch_dmrs,
                          int32_t *txdataF,
                          int16_t amp,
                          uint8_t ssb_start_symbol,
                          nfapi_nr_config_request_scf_t *config,
                          NR_DL_FRAME_PARMS *frame_parms) {
  int k,l;
  //int16_t a;
  int16_t mod_dmrs[NR_PBCH_DMRS_LENGTH<<1];
  uint8_t idx=0;
  uint8_t nushift = config->cell_config.phy_cell_id.value &3;
  LOG_D(PHY, "PBCH DMRS mapping started at symbol %d shift %d\n", ssb_start_symbol+1, nushift);

  /// QPSK modulation
  for (int m=0; m<NR_PBCH_DMRS_LENGTH; m++) {
    idx = (((gold_pbch_dmrs[(m<<1)>>5])>>((m<<1)&0x1f))&3);
    mod_dmrs[m<<1] = nr_qpsk_mod_table[idx<<1];
    mod_dmrs[(m<<1)+1] = nr_qpsk_mod_table[(idx<<1) + 1];
#ifdef DEBUG_PBCH_DMRS
    printf("m %d idx %d gold seq %d b0-b1 %d-%d mod_dmrs %d %d\n", m, idx, gold_pbch_dmrs[(m<<1)>>5], (((gold_pbch_dmrs[(m<<1)>>5])>>((m<<1)&0x1f))&1),
           (((gold_pbch_dmrs[((m<<1)+1)>>5])>>(((m<<1)+1)&0x1f))&1), mod_dmrs[(m<<1)], mod_dmrs[(m<<1)+1]);
#endif
  }

  /// Resource mapping
  // PBCH DMRS are mapped  within the SSB block on every fourth subcarrier starting from nushift of symbols 1, 2, 3
  ///symbol 1  [0+nushift:4:236+nushift] -- 60 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nushift;
  l = ssb_start_symbol + 1;

  for (int m = 0; m < 60; m++) {
#ifdef DEBUG_PBCH_DMRS
    printf("m %d at k %d of l %d\n", m, k, l);
#endif
    ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_dmrs[m<<1]) >> 15;
    ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_dmrs[(m<<1) + 1]) >> 15;
#ifdef DEBUG_PBCH_DMRS
    printf("(%d,%d)\n",
           ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1],
           ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1)+1]);
#endif
    k+=4;

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

  ///symbol 2  [0+u:4:44+nushift ; 192+nu:4:236+nushift] -- 24 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nushift;
  l++;

  for (int m = 60; m < 84; m++) {
#ifdef DEBUG_PBCH_DMRS
    printf("m %d at k %d of l %d\n", m, k, l);
#endif
    ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_dmrs[m<<1]) >> 15;
    ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_dmrs[(m<<1) + 1]) >> 15;
#ifdef DEBUG_PBCH_DMRS
    printf("(%d,%d)\n",
           ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1],
           ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1)+1]);
#endif
    k+=(m==71)?148:4; // Jump from 44+nu to 192+nu

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

  ///symbol 3  [0+nushift:4:236+nushift] -- 60 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nushift;
  l++;

  for (int m = 84; m < NR_PBCH_DMRS_LENGTH; m++) {
#ifdef DEBUG_PBCH_DMRS
    printf("m %d at k %d of l %d\n", m, k, l);
#endif
    ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_dmrs[m<<1]) >> 15;
    ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_dmrs[(m<<1) + 1]) >> 15;
#ifdef DEBUG_PBCH_DMRS
    printf("(%d,%d)\n",
           ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1],
           ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1)+1]);
#endif
    k+=4;

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

#ifdef DEBUG_PBCH_DMRS
  write_output("txdataF_pbch_dmrs.m", "txdataF_pbch_dmrs", txdataF[0], frame_parms->samples_per_frame_wCP>>1, 1, 1);
#endif
  return 0;
}
