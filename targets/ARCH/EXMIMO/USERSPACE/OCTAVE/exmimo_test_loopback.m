% Basic configuration script for ExpressMIMO-1 and ExpressMIMO-2
% Some functions are only available on ExpressMIMO-2!
limeparms;

eNBflag = 0;
card=0;

tdd_config = TXRXSWITCH_LSB;
%tdd_config = TXRXSWITCH_TESTTX;
tdd_config += TEST_ADACLOOP_EN;
syncmode = SYNCMODE_FREE;

%% acquisition
%rf_mode = ( RXEN +        TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM + RXOUTSW )*[1 1 0 0];
 rf_mode = ( RXEN + TXEN + TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM )*[ 1 1 0 0 ]; 

%% Select DMA directions
rf_mode = rf_mode + (DMAMODE_RX + DMAMODE_TX)*[ 1 0 0 0 ];
rf_mode = rf_mode + (DMAMODE_TX )*[0 1 0 0];

freq_rx = (1907600000)*[1 1 1 1];
 rf_local= [rfl(20,25,26,04)   rfl(20,25,26,04)  0                  0]; % 1.9 GHz & 860 MHz, VGA2Gain=0
rf_vcocal=(( 0xE)*(2^6)) + ( 0xE)*[1 1 1 1];  % 1.907 GHz, (OLD)
freq_tx = freq_rx + 0000000; % 1kHz offset: @30 MSPS: 2.5 sines, @15 MSPS: 5 sines, @7 MSPS: 10 sines
tx_gain = 13*[1 1 1 1];
rx_gain = 20*[1 1 1 1]; 
rf_rxdc = (1 + 1*(2^8))*[1 1 1 1]; % I:LSB, Q:MSB
rffe_rxg_low = 61*[1 1 1 1];
rffe_rxg_final = 61*[1 1 1 1];
rffe_band = B19G_TDD*[1 1 1 1];

oarf_config_exmimo(card,freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band)



% create_testvector
vlen=76800
v = floor(( [ (201:200+60000) - i*(101:100+60000) ] ))' - (30000+i*30000);
v = [v; floor(( [ (201:200+vlen-60000) - i*(101:100+vlen-60000) ] ))' - (30000+i*30000) ];

%v=([11:10+vlen] - i*[101:100+vlen])'; % attention, doesn't wrap!
%v=15000*exp(-i*2*pi*1/7.68*[0:vlen-1])';
v2=30000*(1+i)*ones(vlen,2);

oarf_send_frame(card,[v v2],16);

sleep(2); % need this, otherwise, get_frame will return garbage sometimes

%len=1000;off=0; chan=2; hold off ; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold on ; plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",2)
chan=1; s = oarf_get_frame(card); hold off ; grid on; plot(real(s(:,chan)),'b-o',"markersize",2) ;  hold on ; plot(imag(s(:,chan)),'r-o',"markersize",2)

