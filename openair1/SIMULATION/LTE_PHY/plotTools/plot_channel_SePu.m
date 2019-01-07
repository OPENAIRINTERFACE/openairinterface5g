ch_srs_PePu;
ch_srs_PePu;
ch_srs_SePu_0;
ch_srs_SePu_1;
figure(5)
subplot(2,2,1);
plot(abs(ce_srs_SePu_0));
title('precoder, antenna 0');
subplot(2,2,2);
plot(abs(ce_srs_SePu_1));
title('precoder, antenna 1');
subplot(2,2,3);
plot(abs(ce_srs_PePu));
title('srs channel estimates');
subplot(2,2,4);
plot(abs(ce_dl_PePu));
title('dl channel estimates');