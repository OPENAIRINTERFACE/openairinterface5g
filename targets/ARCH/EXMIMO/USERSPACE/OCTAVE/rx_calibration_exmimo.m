close all
clear all
hold off

%gpib_card=0;      % first GPIB PCI card in the computer
%gpib_device=28;   % this is configured in the signal generator Utilities->System->GPIB->Address menu
smbv_ip_addr = "192.168.12.201";
command = [getenv("OPENAIR_TARGETS") "/TEST/ROHDE_SCHWARZ/EthernetRawCommand.out"];

%fc = 1907600e3;
fc = 2.6e9;
%fc=800e6;

fs = 7680e3;
fref = fc+fs/4;

power_dBm      = [-70 -80 -90]+20;
cables_loss_dB = 12;    % we need to account for the power loss between the signal generator and the card input (splitter, cables)

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
%syncmode = SYNCMODE_FREE;
syncmode = [SYNCMODE_MASTER SYNCMODE_SLAVE];
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

for card=1:oarf_get_num_detected_cards
oarf_config_exmimo(card-1,freq_rx,freq_tx,tdd_config,syncmode(card),rx_gain,tx_gain,eNBflag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling_factor);
end
autocal = [0 0 0 0];

ALL_rxrfmode = [LNAByp LNAMed LNAMax];
ALL_gain     = 0:5:30;           

num_chains = 4*oarf_get_num_detected_cards;

SpN0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%SpN1 = zeros(length(ALL_rxrfmode),length(ALL_gain));
N0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%N1 = zeros(length(ALL_rxrfmode),length(ALL_gain));
S0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
S0_lin = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%S1 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
G0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%G1 = zeros(length(ALL_rxrfmode),length(ALL_gain));
NF0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%NF1 = zeros(length(ALL_rxrfmode),length(ALL_gain));
SNR0 = zeros(length(ALL_rxrfmode),length(ALL_gain),num_chains);
%SNR1 = zeros(length(ALL_rxrfmode),length(ALL_gain));

%keyboard

idx_power = 1;
for mode_idx = 1:3
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

       s=oarf_get_frame(0);   
       sleep(.5);

SpN0(idx_power,idx_gain,:) = mean(abs(s).^2,1) - abs(mean(s,1)).^2;
       %SpN1(idx_power,idx_gain) = mean(abs(s(:,2)).^2) - abs(mean(s(:,2))).^2;

       figure(1);
       hold off
       plot(20*log10(abs(fft(s(:,end)))),'r'); %,20*log10(abs(fft(s(:,2)))),'b')
       title("Signal");
       ylim([0 200]);
 
       % noise measurement
       system(sprintf('%s %s ''OUTP:STAT OFF''',command,smbv_ip_addr)); %  deactivate output 
       sleep(.5);

       s=oarf_get_frame(0);   %oarf_get_frame
       sleep(.5);

N0(idx_power,idx_gain,:) = mean(abs(s).^2,1) - abs(mean(s,1)).^2;
       %N1(idx_power,idx_gain) = mean(abs(s(:,2)).^2) - abs(mean(s(:,2))).^2;

       figure(2);
       hold off
       plot(20*log10(abs(fft(s(:,end)))),'r'); %,20*log10(abs(fft(s(:,2)))),'b')
       title("Noise");
       ylim([0 200]);

       % do some plausibility checks
       % if ((N0(idx_power,idx_gain) > SpN0(idx_power,idx_gain)) ||
       %   (N1(idx_power,idx_gain) > SpN1(idx_power,idx_gain)))
       %  error("something is wrong");
       % end

       S0_lin(idx_power,idx_gain,:) = SpN0(idx_power,idx_gain,:)-N0(idx_power,idx_gain,:);
       S0_lin(idx_power,idx_gain,S0_lin(idx_power,idx_gain,:)<0) = 0;
       S0(idx_power,idx_gain,:) = 10*log10(S0_lin(idx_power,idx_gain,:));
       %S1(idx_power,idx_gain) = 10*log10(SpN1(idx_power,idx_gain)-N1(idx_power,idx_gain));
       G0(idx_power,idx_gain,:) = S0(idx_power,idx_gain,:) - power_dBm(mode_idx);
       %G1(idx_power,idx_gain) = S1(idx_power,idx_gain) - power_dBm;
       NF0(idx_power,idx_gain,:) = 10*log10(N0(idx_power,idx_gain,:)) - G0(idx_power,idx_gain,:) + 105;   % 108 is the thermal noise
       %NF1(idx_power,idx_gain) = 10*log10(N1(idx_power,idx_gain)) - G1(idx_power,idx_gain) + 105;
       SNR0(idx_power,idx_gain,:) = S0(idx_power,idx_gain,:)-10*log10(N0(idx_power,idx_gain,:));
       %SNR1(idx_power,idx_gain) = S1(idx_power,idx_gain)-10*log10(N1(idx_power,idx_gain));

       %printf(' %d: Signal strength (%f,%f), Gain (%f %f), N (%f %f) SNR (%f %f) NF (%f %f)\n',
	%      rx_gain(1), S0(idx_power,idx_gain),S1(idx_power,idx_gain),
	%      G0(idx_power,idx_gain),G1(idx_power,idx_gain),
	%      10*log10(N0(idx_power,idx_gain)),10*log10(N1(idx_power,idx_gain)),
	%      SNR0(idx_power,idx_gain),SNR1(idx_power,idx_gain),
	%      NF0(idx_power,idx_gain),NF1(idx_power,idx_gain)); 
       %fflush(stdout);
       %fprintf(fid,'%d, %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n',
       %power_dBm,gain2391,gain9862, S0,S1,G0,G1,10*log10(N0),10*log10(N1),SNR0,SNR1,NF0,NF1); 
 

       figure(3)
       hold off
       plot(ALL_gain,G0(:,:,1),'o-','markersize',10)
       hold on
       plot(ALL_gain,G0(:,:,2),'x-','markersize',10)
       plot(ALL_gain,G0(:,:,3),'s-','markersize',10)
       plot(ALL_gain,G0(:,:,4),'d-','markersize',10)
       legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1','Byp RX2','Med RX2','Max RX2','Byp RX3','Med RX3','Max RX3');
       title('Gains')
       
       figure(4)
       hold off
       plot(ALL_gain,NF0(:,:,1),'o-','markersize',10)
       hold on
       plot(ALL_gain,NF0(:,:,2),'x-','markersize',10)
       plot(ALL_gain,NF0(:,:,3),'s-','markersize',10)
       plot(ALL_gain,NF0(:,:,4),'d-','markersize',10)
       legend('Byp RX0','Med RX0','Max RX0','Byp RX1','Med RX1','Max RX1','Byp RX2','Med RX2','Max RX2','Byp RX3','Med RX3','Max RX3');
       title('Noise Figure')

       idx_gain = idx_gain + 1;

       end

   idx_power = idx_power + 1;

 end

%gpib_send(gpib_card,gpib_device,'OUTP:STAT OFF');         %  deactivate output

%l0 = [ALL_gain2391; ones(size(ALL_gain2391))].'\G0;
%l1 = [ALL_gain2391; ones(size(ALL_gain2391))].'\G1;





