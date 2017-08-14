% Basic configuration script for ExpressMIMO-1 and ExpressMIMO-2
% Some functions are only available on ExpressMIMO-2!
limeparms;

eNBflag = 0;
card=0;

tdd_config = TXRXSWITCH_LSB;
%tdd_config = TXRXSWITCH_TESTTX;
syncmode = SYNCMODE_FREE;

%% acquisition
%rf_mode = ( RXEN +        TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM + RXOUTSW )*[1 1 0 0];
 rf_mode = ( RXEN + TXEN + TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM )*[ 1 0 0 0 ]; 
%rf_mode = rf_mode + (RXEN+TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM )*[ 0 1 0 0 ]; 

% internal loopback test
%rf_mode = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1)*[1 1 0 0];
%rf_mode = (RXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA.1ON+LNAByp+RFBBLNA1)*[1 1 1 1];
%rf_mode = [0 0 0 0];

%% Select DMA directions
%rf_mode = rf_mode + (DMAMODE_RX)*[0 1 0 0];
%rf_mode = rf_mode + (DMAMODE_RX)*[0 0 1 1];
%rf_mode = rf_mode + (DMAMODE_TX)*[1 1 0 0];
rf_mode = rf_mode + (DMAMODE_RX + DMAMODE_TX)*[ 1 0 0 0 ];
%rf_mode = rf_mode + (DMAMODE_RX )*[0 1 0 0];
%rf_mode = rf_mode + (DMAMODE_TX)*[1 0 0 0]; rf_mode = rf_mode + (DMAMODE_RX)*[0 1 0 0];


%freq_rx = 749600000*[1 1 1 1];
%rf_local= [8255004   8253440   8257340   8257340]; % 700 MHz
%rf_vcocal=((0x24)*(2^6)) + (0x24)*[1 1 1 1]; % 700 MHz
freq_rx = (1907600000 - 0*9000)*[1 1 1 1];
%rf_local= [rfl( 0, 0,31,31)   rfl(28,44,31,31)  rfl(60,60,31,31)   rfl(60,60,31,31)]; % 1.9 GHz, OLD
%rf_local= [rfl(56,56,31,31)   rfl(16,12,31,31)  rfl(36,48,31,31)   rfl(32,20,31,31)]; % 1.9 GHz, Exmimo2-No1, lime_cal_exmimo
%rf_local= [rfl(34,40,31,31)   rfl(20,25,38,21)  rfl(12,38,21,14)   rfl(31,19,36, 6)]; % 1.9 GHz, Exmimo2-No1, LIME GUI
 rf_local= [rfl(20,25,26,04)   rfl(20,25,26,04)  0                  0]; % 1.9 GHz & 860 MHz, VGA2Gain=0
%rf_local= [0                  rfl(20,25,38,21)  0                  0                  ]; % 1.9 GHz, VGA2Gain=30

rf_vcocal=(( 0xE)*(2^6)) + ( 0xE)*[1 1 1 1];  % 1.907 GHz, (OLD)
%rf_vcocal=((0x14)*(2^6)) + (0x13)*[1 1 1 1];  % 1.907 GHz, RX(msbs)+ TX(lsbs), RX:0xC..0x14, TX:0xB..0x13 (ExMIMO2-No1)
%rf_vcocal=((0x12)*(2^6)) + (0x10)*[1 1 1 1];  % 1.917 GHz, RX(msbs)+ TX(lsbs), RX:0xD..0x16, TX:0xC..0x14 (ExMIMO2-No1)

%freq_rx = ( 860000000 - 0*9000)*[1 1 1 1];
%rf_vcocal=((0x1d)*(2^6)) + (0x1d)*[1 1 1 1]; % 860 MHz

freq_tx = freq_rx + 0000000; % 1kHz offset: @30 MSPS: 2.5 sines, @15 MSPS: 5 sines, @7 MSPS: 10 sines
tx_gain = 13*[1 1 1 1];
rx_gain = 20*[1 1 1 1]; 

rf_rxdc = rf_rxdc*[1 1 1 1];

rffe_rxg_low = 63*[1 1 1 1];
rffe_rxg_final = 63*[1 1 1 1];
rffe_band = TVWS_TDD*[1 1 1 1];
autocal = [1 1 1 1];
oarf_config_exmimo(card,freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal)
%sleep(0.2)

% stuff for I/Q imbalance correction on RX
%gainimb_rx = -0.0020;  phaseimb_rx = -2.38; % RX IQ imbal. ExMIMO1 / lime1, VGAgain2 = 0, 1.9 GHz
gainimb_rx = -0.0030;  phaseimb_rx = -2.38; % RX IQ imbal. ExMIMO1 / lime1, VGAgain2 = 30, 1.9 GHz

phaseimb_rx = phaseimb_rx/180*pi; % phaser imb in radians
beta_rx = (1/2)*(1 + (1+ gainimb_rx) * exp(1i*phaseimb_rx));
alpha_rx = (1/2)*(1 - (1+ gainimb_rx) * exp(-1i*phaseimb_rx));
den=abs(beta_rx)^2-abs(alpha_rx)^2;
beta_rx=beta_rx/den;
alpha_rx=alpha_rx/den;


% create_testvector
vlen=76800
%v=10000*exp(-i*2*pi*1/7.68*[0:vlen-1]); v=[v;v]'; oarf_send_frame(card,v,16);

%v=([2:1+vlen; 10002:10001+vlen] - i*[3:2+vlen; 10003:10002+vlen])';
%v=32767*(1+i)*ones(vlen,2); oarf_send_frame(card,v,16);
%v = floor(( [640:639+vlen ; 640:639+vlen] / 5 ))'; oarf_send_frame(card,v,16);
%v = floor(( [75000:-1:75001-vlen ; 75000:-1:75001-vlen] / 5 ))'; oarf_send_frame(card,v,16);
%v=i*2^11*ones(vlen,2)+2^11*ones(vlen,2); oarf_send_frame(card,v,16);
%v=15000*exp(-i*2*pi*1/7.68*[0:vlen-1]); v=[v;v]'; oarf_send_frame(card,v,16);
%v=10000*exp(-i*(3000*[0:vlen-1]/vlen)*2*pi); v=[v;v]'; oarf_send_frame(card,v,16);
%v=20000*exp(-i*pi*(0:vlen-1)/2); v=[v;v]'; oarf_send_frame(card,v,16);

% Set LSB to RX (,1) or to TX(,0)
%v=10000*m'.*exp(-i*(50*[0:vlen-1]/vlen)*2*pi); v=bitset(real(v),1,0) + i*bitset(imag(v),1,0); v=[v;v]'; oarf_send_frame(card,v,16);


v2=15000*sin((1:vlen)/vlen*2*pi); v2=[v2;v2]';
v=15000*exp(i*2*pi*1/7.68*[1:vlen]); v=[v;v]';
%oarf_send_frame(card,v,16);

%len=1000;off=0; chan=2; s=oarf_get_frame(card); hold off ; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold on ; plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",2)
%chan=1; s = oarf_get_frame(card); hold off ; plot(real(s(:,chan)),'b-o',"markersize",2) ; hold on ; plot(imag(s(:,chan)),'r-o',"markersize",2)
%while 1;len=5000;off=0; chan=1;  s = oarf_get_frame(card);  hold off ; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold on ; plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",3); sleep(0.3);endwhile
%chan=2; s=oarf_get_frame(card); s=s-0.99999*mean(s(:,chan)); s =  beta_rx.*s + alpha_rx.*conj(s); hold off ; plot(20*log10(abs(fftshift(fft(s(:,chan)))))); grid on; min_r=min(real(s(:,chan))); min_i=min(imag(s(:,chan))); max_r=max(real(s(:,chan))); max_i=max(imag(s(:,chan))); disp("Min-I/Q Max-I/Q"); disp([min_r min_i max_r max_i]);
%chan=2; s=oarf_get_frame(card); s=s-0.99999*mean(s(:,chan)); s =  beta_rx.*s + alpha_rx.*conj(s); hold off ; plot(20*log10(abs(fftshift(fft(hamming(vlen).*s(:,chan)))))); grid on; min_r=min(real(s(:,chan))); min_i=min(imag(s(:,chan))); max_r=max(real(s(:,chan))); max_i=max(imag(s(:,chan))); disp("Min-I/Q Max-I/Q"); disp([min_r min_i max_r max_i]);

