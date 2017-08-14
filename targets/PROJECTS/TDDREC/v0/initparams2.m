# % Author: Mirsad Cirkic
# % Organisation: Eurecom (and Linkoping University)
# % E-mail: mirsad.cirkic@liu.se

%clear
paramsinitialized=false;
limeparms;
rxgain=10;
txgain=25;
eNB_flag = 0;
card = 0;
Ntrx=4;
dual_tx=0;
active_rfA=[1 0 0 0];
active_rfB=[0 1 1 0];
active_rf=active_rfA+active_rfB;

if(active_rfA*active_rfB'!=0) error("The A and B transceive chains must be orthogonal./n") endif

%fc  = 2660000000;
fc  = 1912600000; %1907600000;
%fc = 859.5e6;


%rf_mode=(RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1) * active_rf;
autocal_mode=active_rf;
rf_mode=(TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM) * active_rf;
tdd_config = DUPLEXMODE_FDD+TXRXSWITCH_TESTTX; LSBSWITCH_FLAG=false; 
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
tx_gain = txgain*active_rf;
rx_gain = rxgain*active_rf;
%rx_gain = [10 10 30 0];
oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNB_flag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal_mode);
autocal_mode=0*active_rf; % Autocalibration is only needed the first time we conf. exmimo
amp = pow2(14)-1;
n_bit = 16;
paramsinitialized=true;
