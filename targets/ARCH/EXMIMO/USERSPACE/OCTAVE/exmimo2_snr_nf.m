dual_tx=0;
eNBflag = 0;
card=0;
limeparms;

%% acquisition
%rf_mode = ( RXEN +        TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM + RXOUTSW )*[1 1 0 0];
 rf_mode = ( RXEN + TXEN + TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM )*[1 1 1 1];

% internal loopback test
%rf_mode = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1)*[1 1 0 0];
%rf_mode = (RXEN+     TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1)*[1 1 1 1];
%rf_mode = [0 0 0 0];

%% Select DMA directions
 rf_mode = rf_mode + (DMAMODE_RX + DMAMODE_TX)*[1 0 0 0];

freq_rx = (1907600000 - 0*9000)*[1 1 1 1];
%rf_local= [rfl(56,56,31,31)   rfl(16,12,31,31)  rfl(36,48,31,31)   rfl(32,20,31,31)]; % 1.9 GHz, Exmimo2-No1, lime_cal_exmimo
%rf_local= [rfl(34,40,31,31)   rfl(20,25,38,21)  rfl(12,38,21,14)   rfl(31,19,36, 6)]; % 1.9 GHz, Exmimo2-No1, LIME GUI
 rf_local= [rfl(32,36,26,25)   rfl(20,25,37,21)  rfl(12,38,21,14)   rfl(31,19,36, 6)]; 
 rf_vcocal=(( 0xE)*(2^6)) + ( 0xE)*[1 1 1 1];  % 1.907 GHz, (OLD)
freq_tx = freq_rx + 000000; % 1kHz offset: @30 MSPS: 2.5 sines, @15 MSPS: 5 sines, @7 MSPS: 10 sines
rf_rxdc = rf_rxdc*[1 1 1 1];


 tx_gain = 13*[1 1 1 1]; rx_gain = 25*[1 1 1 1]; amp=32000;
%tx_gain = 25*[1 1 1 1]; rx_gain = 0*[1 1 1 1]; amp=32000;
oarf_config_exmimo(card,freq_rx,freq_tx,0,dual_tx,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal);

sleep(1);
% create_testvector
vlen=76800
chan=1

v=0000*exp(-i*(3*[0:vlen-1]/vlen)*2*pi); v=[v;v]'; oarf_send_frame(card,v,16); sleep(0.2);
s = oarf_get_frame(card); figure ; plot(real(s(:,chan)),'b-.',"markersize",2) ; hold on ; plot(imag(s(:,chan)),'r-.',"markersize",2);
s = s(:,chan) - mean(s(:,chan)); % remove DC
e_s0 = sum( s' * s )
v=amp*exp(-i*(10000*[0:vlen-1]/vlen)*2*pi); e_v=sum(v*v'); v=[v;v]'; oarf_send_frame(card,v,16); sleep(0.2);
s = oarf_get_frame(card); figure ; plot(real(s(:,chan)),'b-.',"markersize",2) ; hold on ; plot(imag(s(:,chan)),'r-.',"markersize",2);
s = s(:,chan) - mean(s(:,chan)); % remove DC
figure; plot(20*log10(fftshift(abs(fft(s))))); grid on;
e_v
e_s = sum( s' * s )
s_over_s0_db = 10*log10( e_s / e_s0 )

