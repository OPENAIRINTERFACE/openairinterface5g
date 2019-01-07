dual_tx=0;
card0=0;
card1=1;
limeparms;
active_rf = zeros(1,4);
active_rf = [1 1 1 1];
autocal = [1 1 1 1];
resampling_factor = [2 2 2 2];

%rf_mode = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM)*[1 1 1 1];
rf_mode = (RXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM)*active_rf;
rf_mode = rf_mode+((DMAMODE_RX)*active_rf);
%freq_rx = 2540000000*[1 1 1 1];
freq_rx = 2600000000*active_rf;
%freq_rx = 2680000000*active_rf; %+ 2540000000*[0 1 0 0]; % + 859500000*[0 0 1 0];
%freq_rx = 1912600000*[1 1 1 1];
%freq_rx = 859500000*[1 1 1 1];
freq_tx = freq_rx;
tx_gain = 0*active_rf; %[1 1 1 1];
rx_gain = 0*active_rf; %1 1 1 1];
rf_local= rf_local*active_rf; %[1 1 1 1];
rf_rxdc = rf_rxdc*active_rf; %[1 1 1 1];
%rf_vcocal=rf_vcocal_859*[1 1 1 1];
rf_vcocal=rf_vcocal_19G*active_rf; %1 1 1 1];
eNBflag = 0;
tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_TESTRX; 
%tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_LSB;
syncmode0 = SYNCMODE_MASTER;
syncmode1 = SYNCMODE_SLAVE;
rffe_rxg_low = 31*active_rf; %[1 1 1 1];
rffe_rxg_final = 63*active_rf; %[1 1 1 1];
rffe_band = B19G_TDD*active_rf; %[1 1 1 1];
printf('Configuring Card 0\n');
oarf_config_exmimo(card0,freq_rx,freq_tx,tdd_config,syncmode0,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling_factor)
sleep(1);
printf('Configuring Card 1\n');
oarf_config_exmimo(card1,freq_rx,freq_tx,tdd_config,syncmode1,rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling_factor)

gpib_card=0;      % first GPIB PCI card in the computer
gpib_device=28;   % this is configured in the signal generator Utilities->System
cables_loss_dB = 6;    % we need to account for the power loss between the signa
power_dBm = -95;

%gpib_send(gpib_card,gpib_device,['POW ' int2str(power_dBm+cables_loss_dB) 'dBm']);
%gpib_send(gpib_card,gpib_device,'OUTP:STAT ON'); %  activate output 
%vlen=76800;
%v = 5000* [ zeros(1,1000) ones(1,500) zeros(1,500) ones(1,1) zeros(1,vlen-1000-1000-1)]; v=[v;v;v;v]'; oarf_send_frame(1,v,16);
%sleep(1)

s=oarf_get_frame(0);
%s1=oarf_get_frame(1);

%s = [s s1];

if (resampling_factor(1)== 2)
length1 = 76800;
f0 = (7.68*(0:length(s(:,1))/4-1)/(length(s(:,1))/4))-3.84;
axis([-3.84,3.84,40,200]);
elseif (resampling_factor(1) == 1)
length1 = 153600;
f0 = (15.36*(0:length(s(:,1))/2-1)/(length(s(:,1))/2))-7.68;
axis([-7.68,7.68,40,200]);
else
length1 = 307200;
f0 = (30.72*(0:length(s(:,1))-1)/(length(s(:,1))))-15.36;
axis([-15.36,15.36,40,200]);
endif


if (resampling_factor(2)== 2)
length2 = 76800;
f1 = (7.68*(0:length(s(:,2))/4-1)/(length(s(:,2))/4))-3.84;
axis([-3.84,3.84,40,200]);
elseif (resampling_factor(2) == 1)
length2 = 153600;
f1 = (15.36*(0:length(s(:,2))/2-1)/(length(s(:,2))/2))-7.68;
axis([-7.68,7.68,40,200]);
else
length2 = 307200;
f1 = (30.72*(0:length(s(:,2))-1)/(length(s(:,2))))-15.36;
axis([-15.36,15.36,40,200]);
endif

if (resampling_factor(3)== 2)
length3 = 76800;
f2 = (7.68*(0:length(s(:,3))/4-1)/(length(s(:,3))/4))-3.84;
axis([-3.84,3.84,40,200]);
elseif (resampling_factor(3) == 1)
length3 = 153600;
f2 = (15.36*(0:length(s(:,3))/2-1)/(length(s(:,3))/2))-7.68;
axis([-7.68,7.68,40,200]);
else
length3 = 307200;
f2 = (30.72*(0:length(s(:,3))-1)/(length(s(:,3))))-15.36;
axis([-15.36,15.36,40,200]);
endif

if (resampling_factor(4)== 2)
length4 = 76800;
f3 = (7.68*(0:length(s(:,4))/4-1)/(length(s(:,4))/4))-3.84;
axis([-3.84,3.84,40,200]);
elseif (resampling_factor(4) == 1)
length4 = 153600;
f3 = (15.36*(0:length(s(:,4))/2-1)/(length(s(:,4))/2))-7.68;
axis([-7.68,7.68,40,200]);
else
length4 = 307200;
f3 = (30.72*(0:length(s(:,4))-1)/(length(s(:,4))))-15.36;
axis([-15.36,15.36,40,200]);
endif

length_slave = 76800;

spec0 = 20*log10(abs(fftshift(fft(s(1:length1,1)))));
spec1 = 20*log10(abs(fftshift(fft(s(1:length2,2)))));
spec2 = 20*log10(abs(fftshift(fft(s(1:length3,3)))));
spec3 = 20*log10(abs(fftshift(fft(s(1:length4,4)))));
spec4 = 20*log10(abs(fftshift(fft(s(1:length_slave,5)))));
spec5 = 20*log10(abs(fftshift(fft(s(1:length_slave,6)))));
spec6 = 20*log10(abs(fftshift(fft(s(1:length_slave,7)))));
spec7 = 20*log10(abs(fftshift(fft(s(1:length_slave,8)))));

axis_time = [1:76800*4];


clf
if (active_rf(1) == 1)
hold off;
plot(f0',spec0,'r');
figure
plot(f0',spec4,'b');
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
hold off;
figure;
plot(real(s(1:length1,1)),'r');
hold on
plot(real(s(1:length_slave,5)),'b');
hold off;
figure;
plot(imag(s(1:length1,1)),'r');
hold on
plot(imag(s(1:length_slave,5)),'b');


%len=2500;off=0; chan=[1 5];  hold off ; figure; plot(real(s(off+1:off+len,chan)),'-o',"markersize",2) ; hold off ; figure;  plot(imag(s(off+1:off+len,chan)),'r-o',"markersize",2);

%hold off
%axis([-3.84,3.84,40,160]);
%gpib_send(gpib_card,gpib_device,'OUTP:STAT OFF'); %  activate output 
%legend('Antenna Port 0','Antenna Port 1','Antenna Port 2','Antenna Port 3');
grid
