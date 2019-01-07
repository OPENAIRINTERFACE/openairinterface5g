dual_tx=1;
cbmimo1=0;

if (cbmimo1)
  oarf_config(0,1,dual_tx,255);
  amp = pow2(7)-1;
  n_bit = 8;
else
  oarf_config_exmimo(1902600000,1,dual_tx,30);
  amp = pow2(15)-1;
  n_bit = 16;
end

s = zeros(76800,2);
s(:,1) = 8*floor(amp*OFDM_TX_FRAME(512,211,128,120,8));
s(:,2) = 8*floor(amp*OFDM_TX_FRAME(512,211,128,120,8));


%s(:,1) = amp * (exp(sqrt(-1)*.5*pi*(0:((76800)-1))));
%s(:,2) = amp * (exp(sqrt(-1)*.5*pi*(0:((76800)-1))));

if (cbmimo1)
  oarf_set_tx_gain(110,110,110,110);
end
oarf_send_frame(0,s,n_bit);

plot(20*log10(abs(fft(s(:,1)))))
