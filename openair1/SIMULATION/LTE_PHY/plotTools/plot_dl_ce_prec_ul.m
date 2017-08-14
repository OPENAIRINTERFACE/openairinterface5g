dl_ch_est_0;
dl_ch_est_1;
ul_precoder_0;
ul_precoder_1;

figure(9)
subplot(2,2,1);
plot(abs(dl_ce_0));
subplot(2,2,2);
plot(abs(dl_ce_1));
subplot(2,2,3);
plot(abs(ul_prec_1));
subplot(2,2,4);
plot(abs(ul_prec_0));


