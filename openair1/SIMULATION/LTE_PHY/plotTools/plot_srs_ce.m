srs_ch_est_0_5;
srs_ch_est_1_5;
srs_ch_est_0_7;
srs_ch_est_1_7;
figure(5)
subplot(2,2,1);
plot(abs(srs_ce_0_5));
title('SRS channel estimate slot 5 antenna 0');
subplot(2,2,2);
plot(abs(srs_ce_1_5));
title('SRS channel estimate slot 5 antenna 1');
subplot(2,2,3);
plot(abs(srs_ce_0_7));
title('SRS channel estimate slot 7 antenna 0');
subplot(2,2,4);
plot(abs(srs_ce_1_7));
title('SRS channel estimate slot 7 antenna 1');