fc  = 2560000000
%fc  = 1907600000;
%fc = 859.5e6;

rxgain=0;
txgain=0;
eNB_flag = 0;
card = 0;
active_rf = [1 0 0 0];
autocal = [1 1 1 1];
resampling_factor = [2 2 2 2];
limeparms;
rf_mode   = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM) * active_rf;
rf_mode = rf_mode + (DMAMODE_RX + DMAMODE_TX)*active_rf;
%rf_mode = rf_mode + (DMAMODE_TX)*active_rf + DMAMODE_RX*active_rf;
%rf_mode   = RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1;
%rf_local= [8253704   8253704   8257340   8257340]; %eNB2tx %850MHz
%rf_local= [8255004   8253440   8257340   8257340]; % ex2 700 MHz
rf_local = [8254744   8255063   8257340   8257340]; %eNB2tx 1.9GHz
%rf_local = [8257292   8257300   8257340   8257340]; %ex2 850 MHz
%rf_local  = rf_local * chan_sel;
rf_rxdc = rf_rxdc * active_rf;
%rf_vcocal = rf_vcocal_859 * chan_sel;
rf_vcocal = rf_vcocal_19G * active_rf;
%rf_vcocal = rf_vcocal_26G_eNB * chan_sel;
rxgain = rxgain*active_rf;
txgain = txgain*active_rf;
freq_tx = fc.*active_rf;
freq_rx = freq_tx;
%freq_rx = freq_tx-120000000*chan_sel;
%freq_tx = freq_rx+1920000;
%tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_LSB;
tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_TESTTX;
syncmode = SYNCMODE_FREE;
rffe_rxg_low = 61*[1 1 1 1];
rffe_rxg_final = 61*[1 1 1 1];
rffe_band = B19G_TDD*[1 1 1 1];

oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,eNB_flag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling_factor);
%oarf_config_exmimo(1, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,eNB_flag,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling_factor);
amp = pow2(10)-1;
n_bit = 16;

length = 307200/pow2(resampling_factor(1));

s = zeros(length,4);

select = 7;

switch(select)

case 0
  s(:,1) = amp * ones(1,length);
  s(:,2) = amp * ones(1,length);
  s(:,3) = amp * ones(1,length);
  s(:,4) = amp * ones(1,length);

case 1
  s(:,1) = floor(amp * (exp(1i*2*pi*(0:((length)-1))/4)));
  s(:,2) = floor(amp * (exp(1i*2*pi*(0:((length)-1))/4)));
  s(:,3) = floor(amp * (exp(1i*2*pi*(0:((length)-1))/4)));
  s(:,4) = floor(amp * (exp(1i*2*pi*(0:((length)-1))/4)));

case 2
  s(38400+128,1)= 80-1j*40;
  s(38400+138,1)= -80+1j*40;

case 3
  for i=0:(12*5-1)
    s((38401+640*i):(38400+640*(i+1)),1)=floor(linspace(-127,128,640)); %+1j*floor(linspace(128,-127,640));
  end

case 4
  pss0_f0=[0,0,0,0,0,0,0,0,0,0,32767,0,-26120,-19785,11971,-30502,-24020,-22288,32117,6492,31311,9658,-16384,-28378,25100,-21063,-7292,-31946,20429,25618,14948,29158,11971,-30502,31311,9658,25100,-21063,-16384,28377,-24020,22287,32117,6492,-7292,31945,20429,25618,-26120,-19785,-16384,-28378,-16384,28377,-26120,-19785,-32402,4883,31311,-9659,32117,6492,-7292,-31946,32767,-1,25100,-21063,-24020,22287,-32402,4883,-32402,4883,-24020,22287,25100,-21063,32767,-1,-7292,-31946,32117,6492,31311,-9659,-32402,4883,-26120,-19785,-16384,28377,-16384,-28378,-26120,-19785,20429,25618,-7292,31945,32117,6492,-24020,22287,-16384,28377,25100,-21063,31311,9658,11971,-30502,14948,29158,20429,25618,-7292,-31946,25100,-21063,-16384,-28378,31311,9658,32117,6492,-24020,-22288,11971,-30502,-26120,-19785,32767,0,0,0,0,0,0,0,0,0,0,0];

  %pss0_f = pss0_f0(1:2:length(pss0_f0)) + sqrt(-1)*pss0_f0(2:2:length(pss0_f0));
  pss0_f = pss0_f0(1:2:size(pss0_f0,2)) + sqrt(-1)*pss0_f0(2:2:size(pss0_f0,2));
  
  pss0_f = [0 pss0_f(37:72) zeros(1,512-73) pss0_f(1:36)];
  pss0_t = ifft(pss0_f);
  pss0_t = [pss0_t((512-127):512) pss0_t];
  pss0_t = [pss0_t zeros(1,512+36) pss0_t]; 
  pss0_max = max(max(abs(real(pss0_t))),max(abs(imag(pss0_t))));
  
  pss0_t_fp_re = floor(real(8192*pss0_t/pss0_max)); 
  pss0_t_fp_im = floor(imag(8192*pss0_t/pss0_max)); 
  
  %keyboard;
  s(38400+(1:length(pss0_t_fp_re)),1) = 2*floor(pss0_t_fp_re) + 2*sqrt(-1)*floor(pss0_t_fp_im);
  s(38400+(1:length(pss0_t_fp_re)),2) = 2*floor(pss0_t_fp_re) + 2*sqrt(-1)*floor(pss0_t_fp_im);
  s(38400+(1:length(pss0_t_fp_re)),3) = 2*floor(pss0_t_fp_re) + 2*sqrt(-1)*floor(pss0_t_fp_im);
  s(38400+(1:length(pss0_t_fp_re)),4) = 2*floor(pss0_t_fp_re) + 2*sqrt(-1)*floor(pss0_t_fp_im);
  %s(38400+(1:size(pss0_t_fp_re,2)),1) = 2*floor(pss0_t_fp_re) + 2*sqrt(-1)*floor(pss0_t_fp_im);
  %s(38400+(1:size(pss0_t_fp_re,2)),2) = 2*floor(pss0_t_fp_re) + 2*sqrt(-1)*floor(pss0_t_fp_im);
  %s(38400+(1:size(pss0_t_fp_re,2)),3) = 2*floor(pss0_t_fp_re) + 2*sqrt(-1)*floor(pss0_t_fp_im);
  %s(38400+(1:size(pss0_t_fp_re,2)),4) = 2*floor(pss0_t_fp_re) + 2*sqrt(-1)*floor(pss0_t_fp_im);

case 5
  x=1:length;

  s(:,1) = 1i*8*(mod(x-1,4096)); % + 1i*(8*(mod(x-1,4096)));
  s(:,2) = 8*(mod(x-1,4096)) + 1i*(8*(mod(x-1,4096)));
  s(:,3) = 8*(mod(x-1,4096)) + 1i*(8*(mod(x-1,4096)));
  s(:,4) = 8*(mod(x-1,4096)) + 1i*(8*(mod(x-1,4096)));

case 6

  nb_rb = 100; %this can be 25, 50, or 100
  num_carriers = 2048/100*nb_rb;
  num_zeros = num_carriers-(12*nb_rb+1);
  prefix_length = num_carriers/4; %this is extended CP
  num_symbols_frame = 120;
  preamble_length = 120;
  
  s(:,1) = OFDM_TX_FRAME(num_carriers,num_zeros,prefix_length,num_symbols_frame,preamble_length);
  s(:,1) = floor(amp*(s(:,1)./max([real(s(:,1)); imag(s(:,1))])));

case 7
  s(:,1) = floor(txs0);

otherwise 
  error('unknown case')

end %switch

s = s*2;

oarf_send_frame(card,s,n_bit);
%oarf_send_frame(1,s,n_bit);
%sleep (1)
%r = oarf_get_frame(card);

figure(1)
hold off
plot(real(s(:,1)),'r')
%hold on
%plot(imag(s(:,2)),'b')
