%if ~exist('dl_ch_est_p_00','var')
  dl_ch_estimate_prim_00;
%end
%if ~exist('dl_ch_est_p_01','var')
  dl_ch_estimate_prim_01;
%end
%if ~exist('dl_ch_est_s_00','var')
  dl_ch_estimate_seco_00;
%end
%if ~exist('dl_ch_est_s_01','var')
  dl_ch_estimate_seco_01;
%end
%if ~exist('dl_ch_est_s_10','var')
  dl_ch_estimate_seco_10;
%end
%if ~exist('dl_ch_est_s_11','var')
  dl_ch_estimate_seco_11;
%end


figure(5)
subplot(3,2,1)
plot(abs(dl_ch_est_p_00));
title('Channel estimate matrix')
subplot(3,2,2)
plot(abs(dl_ch_est_p_01));
title('from Primary eNb')

subplot(3,2,3)
plot(abs(dl_ch_est_s_00));
title('Channel estimate (B/F)');
subplot(3,2,4)
plot(abs(dl_ch_est_s_01));
title('vector from Secondary eNb');
subplot(3,2,5)
plot(abs(dl_ch_est_s_10));
subplot(3,2,6)
plot(abs(dl_ch_est_s_11));

dl_ch_est_0;
dl_ch_est_1;
ul_precoder_0;
ul_precoder_1;
figure(6)
subplot(2,2,1)
plot(abs(dl_ce_0));
subplot(2,2,2)
plot(abs(dl_ce_1));
subplot(2,2,3)
plot(abs(ul_prec_1));
subplot(2,2,4)
plot(abs(ul_prec_0));

figure(7)
subplot(2,4,1)
plot(real(dl_ce_0));
subplot(2,4,2)
plot(imag(dl_ce_0));
subplot(2,4,3)
plot(real(dl_ce_1));
subplot(2,4,4)
plot(imag(dl_ce_1));
subplot(2,4,5)
plot(real(ul_prec_1));
subplot(2,4,6)
plot(imag(ul_prec_1));
subplot(2,4,7)
plot(real(ul_prec_0));
subplot(2,4,8)
plot(imag(ul_prec_0));