embms_20_25;
hold off; semilogy(SNR_20_25,errs_mch_20_25./mch_trials_20_25,'k-+','markersize',10); hold on;
semilogy([20.5],[1e-2],'kx','markersize',20); hold off;
axis([13 21 1e-3 1e-1]);
legend('OAI MBSFN 36.101 R39-1 (5 MHz,MCS20)','R39-1 Minimum Requirement');
xlabel('SNR (dB)');
ylabel('BLER');
title('OAI eMBMS Performance vs. 36-101 Requirements'); 
grid
print -dpng oai_embms_r39-1.png
