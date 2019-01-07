dual_tx=0;
eNBflag = 0;
card=0;
limeparms;

%% acquisition
%rf_mode = ( RXEN +        TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM + RXOUTSW )*[1 1 0 0];
 rf_mode = (        TXEN + TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM )*[1 1 0 0];

% internal loopback test
%rf_mode = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1)*[1 1 0 0];
%rf_mode = (RXEN+     TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1)*[1 1 1 1];
%rf_mode = [0 0 0 0];

%% Select DMA directions
 rf_mode = rf_mode + (DMAMODE_TX)*[0 1 0 0];

freq_rx = (1907600000 - 0*9000)*[1 1 1 1];
%rf_local= [rfl(56,56,31,31)   rfl(16,12,31,31)  rfl(36,48,31,31)   rfl(32,20,31,31)]; % 1.9 GHz, Exmimo2-No1, lime_cal_exmimo
%rf_local= [rfl(34,40,31,31)   rfl(20,25,38,21)  rfl(12,38,21,14)   rfl(31,19,36, 6)]; % 1.9 GHz, Exmimo2-No1, LIME GUI
%rf_local= [rfl(32,36,26,25)   rfl(20,25,37,21)  rfl(12,38,21,14)   rfl(31,19,36, 6)]; 
 rf_local= [rfl(33,36,26,25)   rfl(20,24,37,21)  rfl(12,38,21,14)   rfl(31,19,36, 6)]; 
  rf_vcocal=(( 0xE)*(2^6)) + ( 0xE)*[1 1 1 1];  % 1.907 GHz, (OLD)
freq_tx = freq_rx + 000000; % 1kHz offset: @30 MSPS: 2.5 sines, @15 MSPS: 5 sines, @7 MSPS: 10 sines
rf_rxdc = rf_rxdc*[1 1 1 1];


 tx_gain = 20*[1 1 1 1]; rx_gain = 25*[1 1 1 1]; amp=10000;
%tx_gain = 25*[1 1 1 1]; rx_gain = 0*[1 1 1 1]; amp=32000;
oarf_config_exmimo(card, freq_rx,freq_tx,0,dual_tx,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal);

sleep(1);
% create_testvector
vlen=76800
chan=1

%v=0000*exp(-i*(3*[0:vlen-1]/vlen)*2*pi); v=[v;v]'; oarf_send_frame(card,v,16); sleep(0.2);
%s = oarf_get_frame(card); figure ; plot(real(s(:,chan)),'b-.',"markersize",2) ; hold on ; plot(imag(s(:,chan)),'r-.',"markersize",2);
% create_testvector
vlen=76800
chan=1
amp=10000

%v=0000*exp(-i*(3*[0:vlen-1]/vlen)*2*pi); v=[v;v]'; oarf_send_frame(card,v,16); sleep(0.2);
%s = oarf_get_frame(card); figure ; plot(real(s(:,chan)),'b-.',"markersize",2) ; hold on ; plot(imag(s(:,chan)),'r-.',"markersize",2);

v=amp*exp(-i*(10000*[0:vlen-1]/vlen)*2*pi); v=[v;v]'; 


%for gainimb_tx = -0.05:0.01:0.05
%for phaseimb_tx= -5:1:5

gainimb_tx = -0.028;  % gain imabalance of modulator in linear scale
phaseimb_tx = 3.0; % phase imbalance of modulator in degrees

gainimb_tx = -0.028; phaseimb_tx = 3.0; % for exmimo2-1 lime1

phaseimb_tx = phaseimb_tx/180*pi; % phaser imb in radians
beta_tx = (1/2)*(1 + (1+ gainimb_tx) * exp(1i*phaseimb_tx));
alpha_tx = (1/2)*(1 - (1+ gainimb_tx) * exp(-1i*phaseimb_tx));
den=abs(beta_tx)^2-abs(alpha_tx)^2;
beta_tx=beta_tx/den;
alpha_tx=alpha_tx/den;

v =  beta_tx.*v + alpha_tx.*conj(v); 

sleep(0.5); oarf_send_frame(card,v,16);

