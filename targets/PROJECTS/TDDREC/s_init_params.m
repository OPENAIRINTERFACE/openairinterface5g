clear all
close all
addpath([getenv('OPENAIR_TARGETS') '/ARCH/EXMIMO/USERSPACE/OCTAVE']);
addpath([getenv('OPENAIR_DIR') '/cmake_targets/lte_build_oai/build']);

%% -------- ExpressMIMO2 configuration --------
limeparms;

cardA = 0;
cardB = 1;

v_active_rfA = [1 1 1 1]; 
v_active_rfB = [1 0 0 0];

fc  = 1907600000; %3500000000; %fc = 859.5e6; %fc = 2660000000;
freq_rxA = fc*v_active_rfA; 
freq_txA = freq_rxA; %+1.92e6;
freq_rxB = fc*v_active_rfB; 
freq_txB = freq_rxB; %+1.92e6;

tdd_config = DUPLEXMODE_FDD+TXRXSWITCH_LSB;  %we need the LSB switching for the woduplex script, otherwise we don't receive anything
rx_gainA = 10*v_active_rfA;%[0 0 0 3];
tx_gainA = 10*v_active_rfA;%[20 20 20 20]
rx_gainB = 10*v_active_rfB; 
tx_gainB = 10*v_active_rfB; 
syncmodeA = SYNCMODE_MASTER;
syncmodeB = SYNCMODE_SLAVE;
eNB_flag = 0;
resampling_factorA = [2 2 2 2];%2*v_active_rfA;
resampling_factorB = [2 2 2 2];%2*v_active_rfB;

rf_modeA = (TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM+DMAMODE_TX+TXEN+DMAMODE_RX+RXEN) * v_active_rfA;
rf_rxdcA = rf_rxdc*v_active_rfA;
rf_vcocalA = rf_vcocal_19G*v_active_rfA;
rf_local = [8254744   8255063   8257340   8257340]; %eNB2tx 1.9GHz
rffe_rxg_lowA = 31*v_active_rfA;
rffe_rxg_finalA = 63*v_active_rfA;
rffe_bandA = B19G_TDD*v_active_rfA;
autocal_modeA = v_active_rfA;
oarf_stop(cardA);
sleep(0.1);
oarf_config_exmimo(cardA,freq_rxA,freq_txA,tdd_config,syncmodeA,rx_gainA,tx_gainA,eNB_flag,rf_modeA,rf_rxdcA,rf_local,rf_vcocalA,rffe_rxg_lowA,rffe_rxg_finalA,rffe_bandA,autocal_modeA,resampling_factorA);

rf_modeB = (TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM+DMAMODE_TX+TXEN+DMAMODE_RX+RXEN) * v_active_rfB;
rf_rxdcB = rf_rxdc*v_active_rfB;                        
rf_vcocalB = rf_vcocal_19G*v_active_rfB;
rffe_rxg_lowB = 31*v_active_rfB;
rffe_rxg_finalB = 63*v_active_rfB;
rffe_bandB = B19G_TDD*v_active_rfB;
autocal_modeB = v_active_rfB;
oarf_stop(cardB);
sleep(0.1);
oarf_config_exmimo(cardB,freq_rxB,freq_txB,tdd_config,syncmodeB,rx_gainB,tx_gainB,eNB_flag,rf_modeB,rf_rxdcB,rf_local,rf_vcocalB,rffe_rxg_lowB,rffe_rxg_finalB,rffe_bandB,autocal_modeB,resampling_factorB);

d_n_bit = 16;

