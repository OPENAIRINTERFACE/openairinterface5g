channel_SePu_r_0;
channel_SePu_i_0;
channel_SePu_r_1;
channel_SePu_i_1;
ch_SePu_0 = ce_SePu_r_0 + 1i*ce_SePu_i_0;
ch_SePu_1 = ce_SePu_r_1 + 1i*ce_SePu_i_1;
figure(5)
subplot(1,2,1);
plot(abs(fft(ch_SePu_0,512)));
title('fft of channel in time');
subplot(1,2,2);
plot(abs(fft(ch_SePu_1,512)));
title('fft of channel in time');
