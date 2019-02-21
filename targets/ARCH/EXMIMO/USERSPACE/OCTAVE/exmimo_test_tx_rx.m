dual_tx=0;
eNBflag = 0;
card=0;
limeparms;

% internal loopback test
 rf_mode = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF5+RXLPFNORM+RXLPFEN+RXLPF5+LNA1ON+LNAByp+RFBBLNA1)*[1 1 0 0];
%rf_mode = [0 0 0 0];

%% Select DMA directions
%rf_mode = rf_mode + (DMAMODE_RX)*[1 0 0 0 ];
%rf_mode = rf_mode + (DMAMODE_RX)*[1 1 0 0];
%rf_mode = rf_mode + (DMAMODE_TX)*[1 1 0 0];
%rf_mode = rf_mode + (DMAMODE_RX + DMAMODE_TX)*[1 0 0 0];
 rf_mode = rf_mode + (DMAMODE_RX + DMAMODE_TX)*[1 0 0 0];

 freq_rx = 1907600000*[1 1 1 1];
 rf_local= [8254744   8255063   8257340   8257340]; % 1.9 GHz
 rf_vcocal=((0xE)*(2^6)) + (0xE)*[1 1 1 1];  % 1.9 GHz

freq_tx = freq_rx + 1;
tx_gain = 25*[1 1 1 1];
rx_gain = 20*[1 1 1 1]; 

rf_rxdc = rf_rxdc*[1 1 1 1];

 
oarf_config_exmimo(card, freq_rx,freq_tx,0,dual_tx,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal)

% create_testvector
vlen=76800
%v=([2:1+vlen; 10002:10001+vlen] - i*[3:2+vlen; 10003:10002+vlen])'; % achtung, wrapped nicht!
v = floor(( [640:639+vlen ; 640:639+vlen] / 5 ))';

break

sleep(5)

oarf_send_frame(card,v,16);


%len=1000;off=0; chan=2; hold off ; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold on ; plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",2)
chan=1; s = oarf_get_frame(card); hold off ; plot(real(s(:,chan)),'b',"markersize",1) ; % hold on ; plot(imag(s(:,chan)),'r',"markersize",1)

