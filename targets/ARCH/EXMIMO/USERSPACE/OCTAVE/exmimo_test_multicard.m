% Try out multicard setup
limeparms;

eNBflag = 0;

tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_LSB;
rf_mode = ( RXEN + TXEN + TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM )*[1 0 0 0]; 
rf_mode = rf_mode + (DMAMODE_RX + DMAMODE_TX)*[1 0 0 0];
freq_rx = (1908600000 - 0*9000)*[1 1 1 1];
freq_tx = freq_rx + 0000000; % 1kHz offset: @30 MSPS: 2.5 sines, @15 MSPS: 5 sines, @7 MSPS: 10 sines
rf_local= [ rfl(20,25,26,04) 0 0 0 ]; % 1.9 GHz & 860 MHz, VGA2Gain=0
rf_vcocal=(( 0xE)*(2^6)) + ( 0xE)*[1 1 1 1];  % 1.907 GHz, (OLD)
tx_gain = 13*[1 1 1 1];
rx_gain = 20*[1 1 1 1]; 
rf_rxdc = rf_rxdc*[1 1 1 1];


% set first card to slave mode
card=1; % -1 for all cards
syncmode = SYNCMODE_SLAVE;

oarf_config_exmimo(card,freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal)
sleep(1)

% set second card to master mode
card=0; % -1 for all cards
syncmode = SYNCMODE_MASTER;

oarf_config_exmimo(card,freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal)
sleep(1)

% create_testvector
vlen=76800
%v=5000*exp(-i*(300*[0:vlen-1]/vlen)*2*pi); v=[v;v;v;v]'; oarf_send_frame(0,v,16);
%v=5000*exp(-i*( 30*[0:vlen-1]/vlen)*2*pi); v=[v;v;v;v]'; oarf_send_frame(1,v,16);

%len=1000;off=0; chan=2; s=oarf_get_frame(card); hold off ; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold on ; plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",2)
%while 1;len=5000;off=0; chan=1;  s = oarf_get_frame(card);  hold off ; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold on ; plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",3); sleep(0.3);endwhile
%while 1;len=vlen;off=0; chan=[1 5];  s = oarf_get_frame(-1);  hold off ; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold on ; plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",3); sleep(0.5);endwhile

v = 5000* [ zeros(1,1000) ones(1,500) zeros(1,500) ones(1,1) zeros(1,vlen-1000-1000-1)]; v=[v;v;v;v]'; oarf_send_frame(1,v,16);
sleep(1)

len=2500;off=0; chan=[1 5];  s = oarf_get_frame(-1);  hold off ; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold on ; plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",2);
