typedef unsigned char u8;

/*--------------------------- prototypes --------------------------*/
void f1    ( u8 k[16], u8 rand[16], u8 sqn[6], u8 amf[2],
             u8 mac_a[8] );
void f2345 ( u8 k[16], u8 rand[16],
             u8 res[8], u8 ck[16], u8 ik[16], u8 ak[6] );
void f1star( u8 k[16], u8 rand[16], u8 sqn[6], u8 amf[2],
             u8 mac_s[8] );
void f5star( u8 k[16], u8 rand[16],
             u8 ak[6] );
void ComputeOPc( u8 op_c[16] );
void RijndaelKeySchedule( u8 key[16] );
void RijndaelEncrypt( u8 input[16], u8 output[16] );
