srs_ch_est_0;
srs_ch_est_1;
precoder_0;
precoder_1;
figure(8)
subplot(2,2,1);
plot(abs(srs_ce_0));
title('SRS channel estimate slot 7 antenna 0');
subplot(2,2,2);
plot(abs(srs_ce_1));
title('SRS channel estimate slot 7 antenna 1');
subplot(2,2,3);
plot(abs(prec_0));
title('Precoder antenna 0');
subplot(2,2,4);
plot(abs(prec_1));
title('Precoder antenna 1');

figure(7)
srs_ch_est_0
srs_ch_est_1
subplot(1,2,1)
plot(abs(srs_ce_0))
subplot(1,2,2)
plot(abs(srs_ce_1))
