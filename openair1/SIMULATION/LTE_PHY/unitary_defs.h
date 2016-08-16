openair0_device openair0;
int oai_exit=0;

void exit_fun(const char *s) { exit(-1); }

extern unsigned int dlsch_tbs25[27][25],TBStable[27][110];
extern unsigned char offset_mumimo_llr_drange_fix;

extern unsigned short dftsizes[33];
extern short *ul_ref_sigs[30][2][33];
