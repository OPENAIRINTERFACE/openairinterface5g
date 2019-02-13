dual_tx=0;
card=0;
limeparms;
active_rf = zeros(1,4);
active_rf = [1 1 1 1];
autocal = [1 1 1 1];
resampling_factor = [2 2 2 2];

%rf_mode = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM)*[1 1 1 1];
rf_mode = (RXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM)*active_rf;
rf_mode = rf_mode+((DMAMODE_RX+DMAMODE_TX)*active_rf);
%freq_rx = 2540000000*[1 1 1 1];
freq_rx = 747000000*active_rf;
%freq_rx = 2680000000*active_rf; %+ 2540000000*[0 1 0 0]; % + 859500000*[0 0 1 0];
%freq_rx = 1912600000*[1 1 1 1];
%freq_rx = 859500000*[1 1 1 1];
freq_tx = freq_rx-120000000*active_rf;
tx_gain = 0*active_rf; %[1 1 1 1];
rx_gain = 0*active_rf; %1 1 1 1];
rf_local= rf_local*active_rf; %[1 1 1 1];
rf_rxdc = rf_rxdc*active_rf; %[1 1 1 1];
%rf_vcocal=rf_vcocal_859*[1 1 1 1];
rf_vcocal=rf_vcocal_19G*active_rf; %1 1 1 1];
eNBflag = 0;
tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_TESTRX; 
%tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_LSB;
syncmode = SYNCMODE_FREE;
rffe_rxg_low = 31*active_rf; %[1 1 1 1];
rffe_rxg_final = 63*active_rf; %[1 1 1 1];
rffe_band = B19G_TDD*active_rf; %[1 1 1 1];
oarf_config_exmimo(card,freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling_factor)


gpib_card=0;      % first GPIB PCI card in the computer
gpib_device=28;   % this is configured in the signal generator Utilities->System
cables_loss_dB = 6;    % we need to account for the power loss between the signa
power_dBm = -95;

%gpib_send(gpib_card,gpib_device,['POW ' int2str(power_dBm+cables_loss_dB) 'dBm']);
%gpib_send(gpib_card,gpib_device,'OUTP:STAT ON'); %  activate output 


s=oarf_get_frame(card);
if (resampling_factor(1)== 2)
f0 = (7.68*(0:length(s(:,1))-1)/(length(s(:,1))))-3.84;
axis([-3.84,3.84,40,200]);
elseif (resampling_factor(1) == 1)
f0 = (15.36*(0:length(s(:,1))-1)/(length(s(:,1))))-7.68;
axis([-7.68,7.68,40,200]);
else
f0 = (30.72*(0:length(s(:,1))-1)/(length(s(:,1))))-15.36;
axis([-15.36,15.36,40,200]);
endif


if (resampling_factor(2)== 2)
f1 = (7.68*(0:length(s(:,2))-1)/(length(s(:,2))))-3.84;
axis([-3.84,3.84,40,200]);
elseif (resampling_factor(2) == 1)
f1 = (15.36*(0:length(s(:,2))-1)/(length(s(:,2))))-7.68;
axis([-7.68,7.68,40,200]);
else
f1 = (30.72*(0:length(s(:,2))-1)/(length(s(:,2))))-15.36;
axis([-15.36,15.36,40,200]);
endif

if (resampling_factor(3)== 2)
f2 = (7.68*(0:length(s(:,3))-1)/(length(s(:,3))))-3.84;
axis([-3.84,3.84,40,200]);
elseif (resampling_factor(3) == 1)
f2 = (15.36*(0:length(s(:,3))-1)/(length(s(:,3))))-7.68;
axis([-7.68,7.68,40,200]);
else
f2 = (30.72*(0:length(s(:,3))-1)/(length(s(:,3))))-15.36;
axis([-15.36,15.36,40,200]);
endif

if (resampling_factor(4)== 2)
f3 = (7.68*(0:length(s(:,4))-1)/(length(s(:,4))))-3.84;
axis([-3.84,3.84,40,200]);
elseif (resampling_factor(4) == 1)
f3 = (15.36*(0:length(s(:,4))-1)/(length(s(:,4))))-7.68;
axis([-7.68,7.68,40,200]);
else
f3 = (30.72*(0:length(s(:,4))-1)/(length(s(:,4))))-15.36;
axis([-15.36,15.36,40,200]);
endif


spec0 = 20*log10(abs(fftshift(fft(s(:,1)))));
spec1 = 20*log10(abs(fftshift(fft(s(:,2)))));
spec2 = 20*log10(abs(fftshift(fft(s(:,3)))));
spec3 = 20*log10(abs(fftshift(fft(s(:,4)))));

clf
if (active_rf(1) == 1)
hold off;
plot(f0',spec0,'r');
endif
%hold on
if (active_rf(2) == 1)
hold off;
figure;
plot(f1',spec1,'b');
endif
%hold on
if (active_rf(3) == 1)
hold off;
figure;
plot(f2',spec2,'g');
endif
%hold on
if (active_rf(4) == 1)
hold off;
figure;
plot(f3',spec3,'m');
endif
%hold on

%hold off
%axis([-3.84,3.84,40,160]);
%gpib_send(gpib_card,gpib_device,'OUTP:STAT OFF'); %  activate output 
%legend('Antenna Port 0','Antenna Port 1','Antenna Port 2','Antenna Port 3');
grid
