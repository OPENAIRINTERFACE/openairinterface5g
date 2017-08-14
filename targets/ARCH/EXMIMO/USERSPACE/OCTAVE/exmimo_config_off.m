% Switch off limes
limeparms;

eNBflag = 0;
card=-1;

tdd_config = TXRXSWITCH_LSB;
%tdd_config = TXRXSWITCH_TESTTX;
syncmode = SYNCMODE_FREE;


rf_mode = [0 0 0 0];

%% Select DMA directions
rf_mode = rf_mode + (DMAMODE_RX + DMAMODE_TX)*[ 1 0 0 0 ];
rf_mode = rf_mode + (DMAMODE_RX )*[0 1 0 0];

freq_rx = (1907593260)*[1 1 1 1];
rf_local= [0 0 0 0 ];

rf_vcocal=(( 0xE)*(2^6)) + ( 0xE)*[1 1 1 1];  % 1.907 GHz, (OLD)

freq_tx = freq_rx + 0000000; % 1kHz offset: @30 MSPS: 2.5 sines, @15 MSPS: 5 sines, @7 MSPS: 10 sines
tx_gain = 13*[1 1 1 1];
rx_gain = 10*[1 1 1 1]; 

rf_rxdc = (1 + 1*(2^8))*[1 1 1 1]; % I:LSB, Q:MSB

oarf_config_exmimo(card,freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal)

oarf_stop(-1);
%sleep(0.2)

