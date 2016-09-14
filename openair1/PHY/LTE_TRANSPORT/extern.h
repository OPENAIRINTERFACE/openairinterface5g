extern unsigned int dlsch_tbs25[27][25],TBStable[27][110],TBStable1C[32];
extern unsigned short lte_cqi_eff1024[16];
extern char lte_cqi_snr_dB[15];
extern short conjugate[8],conjugate2[8];
extern short *ul_ref_sigs[30][2][33];
extern short *ul_ref_sigs_rx[30][2][33];
extern unsigned short dftsizes[33];
extern unsigned short ref_primes[33];

extern int qam64_table[8],qam16_table[4];

extern unsigned char cs_ri_normal[4];
extern unsigned char cs_ri_extended[4];
extern unsigned char cs_ack_normal[4];
extern unsigned char cs_ack_extended[4];


extern unsigned char ue_power_offsets[25];

extern unsigned short scfdma_amps[26];

extern char dci_format_strings[15][13];

extern int16_t d0_sss[504*62],d5_sss[504*62];

extern uint8_t wACK[5][4];
extern int8_t wACK_RX[5][4];
