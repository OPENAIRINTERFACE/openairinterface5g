# % Author: Mirsad Cirkic
# % Organisation: Eurecom (and Linkoping University)
# % E-mail: mirsad.cirkic@liu.se

addpath('/homes/kaltenbe/Devel/openair/openair4G/trunk/targets/ARCH/EXMIMO/USERSPACE/OCTAVE');

%clear
paramsinitialized=false;
limeparms;
rxgain=0;
txgain=10;
eNB_flag = 0;
card = 0;
Ntrx=4;
dual_tx=0;
active_rfA=[0 0 0 0];
active_rfB=[1 1 1 1];
active_rf=active_rfA+active_rfB;

if(active_rfA*active_rfB'!=0) error("The A and B transceive chains must be orthogonal./n") endif

%fc  = 2660000000;
fc  = 1917600000;
%fc = 859.5e6;

autocal_mode=active_rf;
%rf_mode=(RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1) * active_rf;
%rf_mode=(TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM) * active_rf;
% we have to enable both DMA transfers so that the switching signal in the LSB of the TX buffer gets set
rf_mode=(TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM+DMAMODE_TX+TXEN+DMAMODE_RX+RXEN) * active_rf;
tdd_config = DUPLEXMODE_FDD+TXRXSWITCH_TESTRX; LSBSWITCH_FLAG=false; %we also need the LSB switching for the woduplex script, otherwise we don't receive anything
%tdd_config = DUPLEXMODE_FDD+TXRXSWITCH_LSB; LSBSWITCH_FLAG=true;
syncmode = SYNCMODE_FREE;
rf_local = [8254744   8255063   8257340   8257340]; %eNB2tx 1.9GHz
rf_vcocal=rf_vcocal_19G*active_rf;

rffe_rxg_low = 61*active_rf;
rffe_rxg_final = 61*active_rf;
rffe_band = B19G_TDD*active_rf;

rf_rxdc = rf_rxdc*active_rf;

freq_rx = fc*active_rf; 
freq_tx = freq_rx; %+1.92e6;
tx_gain = txgain.*active_rf;
rx_gain = rxgain*active_rf;
oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNB_flag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal_mode);
autocal_mode=0*active_rf; % Autocalibration is only needed the first time we conf. exmimo
amp = pow2(14)-1;
n_bit = 16;

chanest_full = 1;

paramsinitialized=true;

