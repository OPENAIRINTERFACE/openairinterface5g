close all
clear all
hold off
card = 0;
% Maxime Guillaud - created Wed May 10 18:08:04 CEST 2006

gpib_card=0;      % first GPIB PCI card in the computer
gpib_device=28;   % this is configured in the signal generator Utilities->System->GPIB->Address menu

cables_loss_dB = 6;    % we need to account for the power loss between the signal generator and the card input (splitter, cables)
dual_tx = 0;
tdd = 1;

fc = 1907600e3;
%fc = 2.68e9;
fs = 7680e3;
%fs = 6500e3;
fref = fc+fs/4;
power_dBm=-70;
f_off_min = 1e6;

limeparms;
%rf_mode = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM)*[1 1 1 1];
rf_mode = (RXEN+0+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM+DMAMODE_RX+0)*[1 0 0 0];
freq_rx = fc*[1 0 0 0];
freq_tx = freq_rx+1920000;
rx_gain = 30*[1 1 1 1];
tx_gain = 25*[1 1 1 1];
%rf_local=rf_local*[1 1 1 1];
rf_local = [8254813 8255016 8254813 8254813]; %exmimo2_2
%rf_rxdc =rf_rxdc*[1 1 1 1];
rf_rxdc = [37059   35459   36300   36999]; %exmimo2_2
rf_vcocal=rf_vcocal_19G*[1 1 1 1];
tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_LSB;
syncmode = SYNCMODE_FREE;
rffe_rxg_low = 63*[1 1 1 1];
rffe_rxg_final = 31*[1 1 1 1];
rffe_band = B19G_TDD*[1 1 1 1];
autocal = [1 1 1 1];
resampling = [2 2 2 2];

%gpib_send(gpib_card,gpib_device,'*RST;*CLS');   % reset and configure the signal generator
%gpib_send(gpib_card,gpib_device,['POW ' int2str(power_dBm+cables_loss_dB) 'dBm']);
%gpib_send(gpib_card,gpib_device,['FREQ 1.91860 Ghz']); % set the frequency 
%gpib_send(gpib_card,gpib_device,['FREQ ' int2str(fref/1e3) 'khz']); % set the frequency 
%gpib_send(gpib_card,gpib_device,'OUTP:STAT ON'); % activate output 

%keyboard;



sleep(2)

step = 4096;
i=0;
do 
  format long
  fc
  freq_rx = fc*[1 0 0 0];
  freq_tx = freq_rx+1920000;
  oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,0,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling);

  i=i+1;
  sleep(1);
  s=oarf_get_frame(card);   %oarf_get_frame
  s(76801:end,:)=[];

  nb_rx = 1; %size(s,2);

  % find the DC component
  m = mean(s);

  s_phase = unwrap(angle(s(10000:4:length(s),1).'));
  s_phase = s_phase - s_phase(1,1);
  f_off = mean(s_phase(2:length(s_phase))*fs/4./(1:(length(s_phase)-1))/2/pi)
  plot(1:length(s_phase),s_phase,'r');

  if (nb_rx>1)
    s_phase2 = unwrap(angle(s(10000:4:length(s),2).'));
    s_phase2 = s_phase2 - s_phase2(1,1);
    f_off2 = mean(s_phase2(2:length(s_phase2))*fs/4./(1:(length(s_phase2)-1))/2/pi)
    hold on
    plot(1:length(s_phase2),s_phase2,'g');
    drawnow;
    hold off
  end

  
  if (abs(f_off) < f_off_min)
      f_off_min = abs((f_off));
  end

  if ((f_off) > 0)
    fc = fc + step;
  else
    fc = fc - step;
  endif

  step = step/2;
until (step < 50)


%gpib_send(gpib_card,gpib_device,'OUTP:STAT OFF');         %  deactivate output


