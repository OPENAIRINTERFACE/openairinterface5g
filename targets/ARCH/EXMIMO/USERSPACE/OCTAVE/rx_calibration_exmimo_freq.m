close all
clear all
hold off

%gpib_card=0;      % first GPIB PCI card in the computer
%gpib_device=28;   % this is configured in the signal generator Utilities->System->GPIB->Address menu
smbv_ip_addr = "192.168.12.201";
command = [getenv("OPENAIR_TARGETS") "/TEST/ROHDE_SCHWARZ/EthernetRawCommand.out"];

fc = 1907600e3;
%fc = 2.6e9;
%fc=700e6;

fs = 7680e3;
fref = fc+fs/4;

power_dBm      = [-70 -80 -90]; %needs to be adjusted based on carrier freq
cables_loss_dB = 11;    % we need to account for the power loss between the signal generator and the card input (splitter, cables)
nb_meas = 10;

dual_tx = 0;
tdd = 1;
card = 0;
limeparms;
%rf_mode = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM+DMAMODE_RX+DMAMODE_TX)*[1 1 1 1];
rf_mode = (RXEN+0+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM+DMAMODE_RX+0)*[1 1 1 1];
freq_rx = fc*[1 1 1 1];
freq_tx = freq_rx;
tx_gain = 25*[1 1 1 1];
rx_gain = 15*[1 1 1 1];
%rf_local= [8254744   8255063   8257340   8257340]; %rf_local*[1 1 1 1];
rf_local = [8254813 8255016 8254813 8254813]; %exmimo2_2
%rf_rxdc = rf_rxdc*[1 1 1 1];
%rf_rxdc   = ((128+rxdc_I) + (128+rxdc_Q)*(2^8))*[1 1 1 1];
rf_rxdc = [37059   35459   36300   36999]; %exmimo2_2
rf_vcocal=rf_vcocal_19G*[1 1 1 1];
eNBflag = 0;
tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_TESTRX;
syncmode = SYNCMODE_FREE;
%syncmode = [SYNCMODE_MASTER SYNCMODE_SLAVE];
rffe_rxg_low = 63*[1 1 1 1];
rffe_rxg_final = 63*[1 1 1 1];
rffe_band = B19G_TDD*[1 1 1 1];
autocal = [1 1 1 1];
resampling_factor = [2 2 2 2];


system(sprintf('%s %s ''*RST;*CLS''',command, smbv_ip_addr));   % reset and configure the signal generator
%gpib_send(gpib_card,gpib_device,sprintf("POW %ddBm",power_dBm+cables_loss_dB));
%gpib_send(gpib_card,gpib_device,'POW -14dBm');
%gpib_send(gpib_card,gpib_device,'FREQ 1.91860GHz');
%gpib_send(gpib_card,gpib_device,'FREQ 1.919225GHz');
%gpib_send(gpib_card,gpib_device,'FREQ 1.909225GHz');
system(sprintf('%s %s ''FREQ %ldHz''',command,smbv_ip_addr,fref));

%for card=1:oarf_get_num_detected_cards
%oarf_config_exmimo(card-1,freq_rx,freq_tx,tdd_config,syncmode(card),rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling_factor);
%end

autocal = [0 0 0 0];

ALL_rxrfmode = [LNAByp LNAMed LNAMax];
%ALL_rxrfmode = [LNAMax];
ALL_gain     = 0:10:30;           

num_chains = 4*oarf_get_num_detected_cards;

SpN0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%SpN1 = zeros(length(ALL_rxrfmode),length(ALL_gain));
N0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%N1 = zeros(length(ALL_rxrfmode),length(ALL_gain));
S0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
S0_lin = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%S1 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
G0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
G1 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
NF0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
NF1 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
NF2 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
SNR0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%SNR1 = zeros(length(ALL_rxrfmode),length(ALL_gain));

%keyboard

for mode_idx = 3
  LNA=ALL_rxrfmode(mode_idx);

  system(sprintf('%s %s ''POW %d dBm''',command,smbv_ip_addr,power_dBm(mode_idx)+cables_loss_dB));

  idx_gain = 1;  
  for rx_gain=ALL_gain
  
       rf_mode = (RXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNA+RFBBNORM+DMAMODE_RX)*[1 1 1 1];
       rx_gain = rx_gain * [1 1 1 1];

       for card=1:oarf_get_num_detected_cards
           oarf_config_exmimo(card-1,freq_rx,freq_tx,tdd_config,syncmode(card),rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling_factor);
       end
       sleep(1);

       % signal measurement
       system(sprintf('%s %s ''OUTP:STAT ON''',command,smbv_ip_addr)); %  activate output 
       sleep(.5);
       %f1 = (7.68*(0:length(s(:,2))-1)/(length(s(:,2))))-3.84;
       f1 = (7.68*(0:307200-1)/307200)-3.84;
       %nidx = find(f1>=0.5 & f1<=1.5); %1MHz 
       nidx = find(f1>=1.5 & f1<=2.5); %1MHz 
       sidx = find(f1>=1.87 & f1<=1.97); %100kHz

       SpN0_temp = 0;
       spn0_temp = 0;
       for k=1:nb_meas
           s=oarf_get_frame(0);   
           sleep(.5);
           s = s - repmat(mean(s,1),size(s,1),1);

           SpN0_temp = SpN0_temp + mean(abs(s).^2,1) - abs(mean(s,1)).^2;
           sf = fftshift(fft(s),1)/sqrt(length(s));
           %np = 10*log10(sum(abs(sf(nidx,:)).^2));
           %sp_temp = sp_temp + 10*log10(sum(abs(sf(sidx,:)).^2));
           spn0_temp = spn0_temp + sum(abs(sf).^2);
       end
       %SpN0(mode_idx,idx_gain,:) = mean(abs(s).^2,1) - abs(mean(s,1)).^2;
       %SpN1(mode_idx,idx_gain) = mean(abs(s(:,2)).^2) - abs(mean(s(:,2))).^2;
       SpN0(mode_idx,idx_gain,:) = SpN0_temp/nb_meas; 
       spn0 = spn0_temp/nb_meas;

       %keyboard;

       sf = fftshift(fft(s),1)/sqrt(length(s));
       figure(1);
       hold off
       plot(f1,20*log10(abs((sf(:,3)))),'r'); 
       hold on
       plot(f1(nidx),20*log10(abs(sf(nidx,3))),'b');
       plot(f1(sidx),20*log10(abs(sf(sidx,3))),'g');

       title("Signal");
       ylim([0 200]);
 
       % noise measurement
       system(sprintf('%s %s ''OUTP:STAT OFF''',command,smbv_ip_addr)); %  deactivate output 
       sleep(.5);

       N0_temp = 0;
       np_temp = 0;
       np_temp_unit = 0;
       for k=1:nb_meas
       	   s=oarf_get_frame(0);   %oarf_get_frame
           sleep(.5);
           s = s - repmat(mean(s,1),size(s,1),1);

           N0_temp = N0_temp + mean(abs(s).^2,1) - abs(mean(s,1)).^2;
	   %N1(idx_power,idx_gain) = mean(abs(s(:,2)).^2) - abs(mean(s(:,2))).^2;
           sf = fftshift(fft(s),1)/sqrt(length(s));
           np_temp = np_temp + sum(abs(sf).^2);
           np_temp_unit = np_temp_unit + sum(abs(sf(nidx,:)).^2);
       end
       N0(mode_idx,idx_gain,:) = N0_temp/nb_meas;         
       %N0(mode_idx,idx_gain,:) = mean(abs(s).^2,1) - abs(mean(s,1)).^2;
       %N1(mode_idx,idx_gain) = mean(abs(s(:,2)).^2) - abs(mean(s(:,2))).^2;
       %G1(mode_idx,idx_gain,:) =  sp - power_dBm(mode_idx);
       np = np_temp/nb_meas;
       snr = 10*log10((spn0-np)./np); %measured snr
       snr_real = power_dBm(mode_idx) - (-174 + 10*log10(7.68e6)); %signal over thermal noise

       np_unit = np_temp_unit/nb_meas;
       snr_output_unit = 10*log10((spn0-np_unit)./np_unit); %measured snr
       snr_input_unit = power_dBm(mode_idx) - (-174 + 10*log10(1e6)); %signal over thermal noise

       NF1(mode_idx,idx_gain,:) = snr_real - snr;
       NF2(mode_idx,idx_gain,:) = snr_input_unit - snr_output_unit;

       sf = fftshift(fft(s),1)/sqrt(length(s));
       figure(2);
       hold off
       plot(f1,20*log10(abs((sf(:,3)))),'r'); 
       title("Noise");
       ylim([0 200]);

       % do some plausibility checks
       % if ((N0(mode_idx,idx_gain) > SpN0(mode_idx,idx_gain)) ||
       %   (N1(mode_idx,idx_gain) > SpN1(mode_idx,idx_gain)))
       %  error("something is wrong");
       % end

       S0_lin(mode_idx,idx_gain,:) = SpN0(mode_idx,idx_gain,:)-N0(mode_idx,idx_gain,:);
       S0_lin(mode_idx,idx_gain,S0_lin(mode_idx,idx_gain,:)<0) = 0;
       S0(mode_idx,idx_gain,:) = 10*log10(S0_lin(mode_idx,idx_gain,:));
       %S1(mode_idx,idx_gain) = 10*log10(SpN1(mode_idx,idx_gain)-N1(mode_idx,idx_gain));
       G0(mode_idx,idx_gain,:) = S0(mode_idx,idx_gain,:) - power_dBm(mode_idx);
       %G1(mode_idx,idx_gain) = S1(mode_idx,idx_gain) - power_dBm;
       NF0(mode_idx,idx_gain,:) = 10*log10(N0(mode_idx,idx_gain,:)) - G0(mode_idx,idx_gain,:) + 105;   % 108 is the thermal noise
       %NF1(mode_idx,idx_gain) = 10*log10(N1(mode_idx,idx_gain)) - G1(mode_idx,idx_gain) + 105;
       SNR0(mode_idx,idx_gain,:) = S0(mode_idx,idx_gain,:)-10*log10(N0(mode_idx,idx_gain,:));
       %SNR1(mode_idx,idx_gain) = S1(mode_idx,idx_gain)-10*log10(N1(mode_idx,idx_gain));

       %printf(' %d: Signal strength (%f,%f), Gain (%f %f), N (%f %f) SNR (%f %f) NF (%f %f)\n',
	%      rx_gain(1), S0(mode_idx,idx_gain),S1(mode_idx,idx_gain),
	%      G0(mode_idx,idx_gain),G1(mode_idx,idx_gain),
	%      10*log10(N0(mode_idx,idx_gain)),10*log10(N1(mode_idx,idx_gain)),
	%      SNR0(mode_idx,idx_gain),SNR1(mode_idx,idx_gain),
	%      NF0(mode_idx,idx_gain),NF1(mode_idx,idx_gain)); 
       %fflush(stdout);
       %fprintf(fid,'%d, %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n',
       %power_dBm,gain2391,gain9862, S0,S1,G0,G1,10*log10(N0),10*log10(N1),SNR0,SNR1,NF0,NF1); 
       %keyboard 

       figure(3)
       hold off
       plot(ALL_gain,G0(:,:,1),'o-','markersize',10)
       hold on
       plot(ALL_gain,G0(:,:,2),'x-','markersize',10)
       plot(ALL_gain,G0(:,:,3),'s-','markersize',10)
       plot(ALL_gain,G0(:,:,4),'d-','markersize',10)
       %legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1');
       legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1','Byp RX2','Med RX2','Max RX2','Byp RX3','Med RX3','Max RX3');
       title('Gains')
       xlabel('rx gains (dB)')
       ylabel('NF (dB)')
       
       figure(4)
       hold off
       plot(ALL_gain,NF0(:,:,1),'o-','markersize',10)
       hold on
       plot(ALL_gain,NF0(:,:,2),'x-','markersize',10)
       plot(ALL_gain,NF0(:,:,3),'s-','markersize',10)
       plot(ALL_gain,NF0(:,:,4),'d-','markersize',10)
       %legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1');
       legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1','Byp RX2','Med RX2','Max RX2','Byp RX3','Med RX3','Max RX3');
       title('Averaged Noise Figure measured in time domain')
       xlabel('rx gains (dB)')
       ylabel('NF (dB)')

       figure(5)
       hold off
       plot(ALL_gain,NF1(:,:,1),'o-','markersize',10)
       hold on
       plot(ALL_gain,NF1(:,:,2),'x-','markersize',10)
       plot(ALL_gain,NF1(:,:,3),'s-','markersize',10)
       plot(ALL_gain,NF1(:,:,4),'d-','markersize',10)
       %legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1');
       legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1','Byp RX2','Med RX2','Max RX2','Byp RX3','Med RX3','Max RX3');
       title('Averaged Noise Figure measured in frequency domain')
       xlabel('rx gains (dB)')
       ylabel('NF (dB)')


       figure(6)
       hold off
       plot(ALL_gain,NF2(:,:,1),'o-','markersize',10)
       hold on
       plot(ALL_gain,NF2(:,:,2),'x-','markersize',10)
       plot(ALL_gain,NF2(:,:,3),'s-','markersize',10)
       plot(ALL_gain,NF2(:,:,4),'d-','markersize',10)
       %legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1');
       legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1','Byp RX2','Med RX2','Max RX2','Byp RX3','Med RX3','Max RX3');
       title('Unit Bandwidth Noise Figure measured in frequency domain')
       xlabel('rx gains (dB)')
       ylabel('NF (dB)')

       idx_gain = idx_gain + 1;

       end

 end

%gpib_send(gpib_card,gpib_device,'OUTP:STAT OFF');         %  deactivate output

%l0 = [ALL_gain2391; ones(size(ALL_gain2391))].'\G0;
%l1 = [ALL_gain2391; ones(size(ALL_gain2391))].'\G1;





