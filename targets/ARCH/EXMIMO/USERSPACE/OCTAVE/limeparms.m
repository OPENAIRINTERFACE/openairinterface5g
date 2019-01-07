% Octave defines, according to pcie_interface.h

% Parameters for rf_mode in structure exmimo_rf_t

RXEN=1;
TXEN=2;

TXLPFENMASK=4;
TXLPFEN=4;


TXLPFMASK    =15*(2^3);
TXLPF14      =0;
TXLPF10      =1*(2^3);
TXLPF7       =2*(2^3);
TXLPF6       =3*(2^3);
TXLPF5       =4*(2^3);
TXLPF4375    =5*(2^3);
TXLPF35      =6*(2^3);
TXLPF3       =7*(2^3);
TXLPF275     =8*(2^3);
TXLPF25      =9*(2^3);
TXLPF192     =10*(2^3);
TXLPF15      =11*(2^3);
TXLPF1375    =12*(2^3);
TXLPF125     =13*(2^3);
TXLPF0875    =14*(2^3);
TXLPF075     =15*(2^3);
RXLPFENMASK=1*(2^7);
RXLPFEN    =128;
RXLPFMASK  =15*(2^8);
RXLPF14    =0;
RXLPF10    =1*(2^8);
RXLPF7     =2*(2^8);
RXLPF6     =3*(2^8);
RXLPF5     =4*(2^8);
RXLPF4375  =5*(2^8);
RXLPF35    =6*(2^8);
RXLPF3     =7*(2^8);
RXLPF275   =8*(2^8);
RXLPF25    =9*(2^8);
RXLPF192   =10*(2^8);
RXLPF15    =11*(2^8);
RXLPF1375  =12*(2^8);
RXLPF125   =13*(2^8);
RXLPF0875  =14*(2^8);
RXLPF075   =15*(2^8);
LNAMASK=3*(2^12);
LNADIS =0;
LNA1ON =1*(2^12);
LNA2ON =2*(2^12) ;
LNA3ON =3*(2^12);
LNAGAINMASK=3*(2^14);
LNAByp    =1*(2^14);
LNAMed    =2*(2^14);
LNAMax    =3*(2^14);

RFBBMASK  =7*(2^16);
RFBBNORM  =0;
RFBBRXLPF =1*(2^16);
RFBBRXVGA =2*(2^16);
RFBBOUTPUT=3*(2^16);
RFBBLNA1  =4*(2^16);
RFBBLNA2  =5*(2^16);
RFBBLNA3  =6*(2^16);

RXLPFMODEMASK=3*(2^19);
RXLPFNORM    =0;
RXLPFBYP     =1*(2^19);
RXLPFBPY2    =3*(2^19);

TXLPFMODEMASK=1*(2^21);
TXLPFNORM    =0;
TXLPFBYP     =1*(2^21);

RXOUTSW      =1*(2^22);

DMAMODE_TRXMASK =3*(2^23);
DMAMODE_RX      =1*(2^23);
DMAMODE_TX      =2*(2^23);

rf_local  = 31 + 31*(2^6) + 31*(2^12) + 31*(2^18);
rf_rxdc   = 0 + 0*(2^8); % DC offset ( DCOFF_I_RXFE [6:0], DCOFF_Q_RXFE[14:8] )
rf_vcocal_19G = ((0xE)*(2^6)) + (0xE); % VCO calibration values for 1.9 GHz
rf_vcocal_700 = ((0x24)*(2^6)) + (0x24); % 700 MHz
rf_vcocal_850 = ((0x1d)*(2^6)) + (0x1d); %850 MHz
rf_vcocal_859 = ((31)*(2^6)) + (31); %859.5 MHz
%rf_vcocal_26G_eNB = ((0x1f)*(2^6)) + (0x13); 
%rf_vcocal_26G_UE  = ((0x13)*(2^6)) + (0x1f);
rf_vcocal_26G_UE = ((0x3)*(2^6)) + (0x26); 
rf_vcocal_26G_eNB  = ((0x26)*(2^6)) + (0x0); 

% register values and masks for tdd_config
DUPLEXMODE_MASK   = 1*(2^0);
DUPLEXMODE_FDD    =  0;
DUPLEXMODE_TDD    = 2*(2^0);
TXRXSWITCH_MASK   = 3*(2^1);
TXRXSWITCH_LSB    =  0;
TXRXSWITCH_FPGA   = 1*(2^1);
TXRXSWITCH_TESTRX = 2*(2^1);
TXRXSWITCH_TESTTX = 3*(2^1);
SWITCHSTATE_MASK  = 1*(2^3);
SWITCHSTATE_0     =  0;
SWITCHSTATE_1     = 1*(2^3);
TEST_ADACLOOP_MASK= 1*(2^4);
TEST_ADACLOOP_EN  = 1*(2^4);

% multicard synchronization mode (multicard_syncmode)
SYNCMODE_FREE    = 0;
SYNCMODE_MASTER  = 1;
SYNCMODE_SLAVE   = 2;

% external RF frontend 
TVWS_TDD = 0;
DD_FDD   = 1;    
DD_TDD   = 2;  
B19G_TDD = 3;  
B24G_TDD = 4;
B26G_TDD = 5;
B26G_FDD = 6;
B35G_TDD = 7;
B50G_TDD = 8;