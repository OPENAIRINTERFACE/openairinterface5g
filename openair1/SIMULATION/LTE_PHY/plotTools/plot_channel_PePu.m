channel_PePu_r;
channel_PePu_i;
ch_srs_PePu;
ch_dl_PePu;
ch_cp = ce_PePu_r + 1i*ce_PePu_i;
figure(5)
subplot(3,1,1);
plot(abs(fft(ch_cp,512)));
title('fft of channel in time');
subplot(3,1,2);
plot(abs(ce_srs_PePu));
title('srs channel estimates');
subplot(3,1,3);
plot(abs(ce_dl_PePu));
title('dl channel estimates');