%
% SCRIPT ID : s_beamforming
%
% PROJECT NAME : TDD Recoprocity
%
% PURPOSE : perform beamforming based on TDD calibration
%
%**********************************************************************************************
%                            Eurecom -  All rights reserved
%
% AUTHOR(s) : Xiwen JIANG, Florian Kaltenberger
%
% DEVELOPMENT HISTORY :
%
% Date         Name(s)       Version  Description
% -----------  ------------- -------  ------------------------------------------------------
% Apr-30-2014  X. JIANG       0.1     script creation v0.1
%
% REFERENCES/NOTES/COMMENTS :
%
% - Based on the script "beamforming" created by Mirsad Cirkic, Florian Kaltenberger.
% 
%**********************************************************************************************

%% -------- initilisation --------
d_M = 4;			% modulation order, e.g. 4 means QPSK

%** frequency **
d_fc  = 1907600000; 
d_delta_f = 15000;
d_N_f = 301; 			% carrier number carrying data
d_N_FFT = 512;			% total carrier number
d_N_CP = 128;			% extented cyclic prefix
%** time **
d_N_OFDM = 60;			% number of ofdm symbol per half frame
d_N_meas = 1;			% measuement number
%** space **
d_N_antA = 4;			% antenna number at site a
d_N_antB = 4; 			% antenna number at site b
v_active_rfA = [1 1 1 1]; 
v_active_rfB = [1 0 0 0];
v_indA = find(v_active_rfA);	% active antenna index at site a
v_indB = find(v_active_rfB);	% active antenna index at site b
d_N_antA_act = length(v_indA);
d_N_antB_act = length(v_indB);

%** amplitude **
d_amp = pow2(12.5)-1;  

%% -------- load F -------- 
o_result = load('result/4a_l45_t10b.mat');
m_F_full = o_result.m_F0;
m_F_diag = o_result.m_F2;

%% -------- channel measurement -------- 
s_run_meas;

%% -------- signal precoding -------- 
v_MPSK = exp(sqrt(-1)*([1:d_M]*2*pi/d_M+pi/d_M));						
m_sym_TA = v_MPSK(ceil(rand(d_N_f, d_N_OFDM*2)*d_M));   

m_sym_TA_ideal = zeros(d_N_f,d_N_OFDM*2,d_N_antA_act);

for d_f = 1:d_N_f
    %** ideal case **
    v_H_A2B_ideal = squeeze(m_H_est_A2B(:,:,d_f));
    v_P_ideal = v_H_A2B_ideal'/norm(v_H_A2B_ideal);
    m_sym_TA_ideal(d_f,:,:) = (v_P_ideal*m_sym_TA(d_f,:)).';
    %** identity matrix **
    v_H_A2B_iden = squeeze(m_H_est_B2A(:,:,d_f)).';
    v_P_iden = v_H_A2B_iden'/norm(v_H_A2B_iden);
    m_sym_TA_iden(d_f,:,:) = (v_P_iden*m_sym_TA(d_f,:)).';
    %** diagonal calibration **
    v_H_A2B_diag = squeeze(m_H_est_B2A(:,:,d_f).')*squeeze(m_F_diag(:,:,2));
    v_P_diag = v_H_A2B_diag'/norm(v_H_A2B_diag);
    m_sym_TA_diag(d_f,:,:) = (v_P_diag*m_sym_TA(d_f,:)).';
    %** full calibration **
    v_H_A2B_full = squeeze(m_H_est_B2A(:,:,d_f).')*squeeze(m_F_diag(:,:,2));
    v_P_full = v_H_A2B_full'/norm(v_H_A2B_full);
    m_sym_TA_full(d_f,:,:) = (v_P_full*m_sym_TA(d_f,:)).';
end

%% -------- signal transmission -------- 
m_sig_TA_ideal = f_ofdm_mod(m_sym_TA_ideal,d_N_FFT,d_N_CP,d_N_OFDM*2,v_active_rfA,d_amp)*2;
m_sig_TA_iden = f_ofdm_mod(m_sym_TA_iden,d_N_FFT,d_N_CP,d_N_OFDM*2,v_active_rfA,d_amp)*2;
m_sig_TA_diag = f_ofdm_mod(m_sym_TA_diag,d_N_FFT,d_N_CP,d_N_OFDM*2,v_active_rfA,d_amp)*2;
m_sig_TA_full = f_ofdm_mod(m_sym_TA_full,d_N_FFT,d_N_CP,d_N_OFDM*2,v_active_rfA,d_amp)*2;

m_sig_TB = zeros((d_N_CP+d_N_FFT)*d_N_OFDM*2,d_N_antB);
oarf_send_frame(cardB,m_sig_TB,d_n_bit);
m_noise_RB_ = oarf_get_frame(cardB);
m_noise_RB = m_noise_RB_(1:d_N_OFDM*(d_N_FFT+d_N_CP),5:8); 
m_n_sym_RB = f_ofdm_rx(m_noise_RB, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfB);

m_sig_TB(:,:)=1+1i;
oarf_send_frame(cardB,m_sig_TB,d_n_bit);

oarf_send_frame(cardA,m_sig_TA_ideal,d_n_bit);
m_sig_RB_ideal_ = oarf_get_frame(cardB);

m_sig_RB_ideal = m_sig_RB_ideal_(1:d_N_OFDM*(d_N_FFT+d_N_CP),5:8); 
m_sym_RB_ideal = f_ofdm_rx(m_sig_RB_ideal, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfB);

oarf_send_frame(cardA,m_sig_TA_iden,d_n_bit);
m_sig_RB_iden_ = oarf_get_frame(cardB);
m_sig_RB_iden = m_sig_RB_iden_(1:d_N_OFDM*(d_N_FFT+d_N_CP),5:8); 
m_sym_RB_iden = f_ofdm_rx(m_sig_RB_iden, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfB);

oarf_send_frame(cardA,m_sig_TA_diag,d_n_bit);
m_sig_RB_diag_ = oarf_get_frame(cardB);
m_sig_RB_diag = m_sig_RB_diag_(1:d_N_OFDM*(d_N_FFT+d_N_CP),5:8); 
m_sym_RB_diag = f_ofdm_rx(m_sig_RB_diag, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfB);

oarf_send_frame(cardA,m_sig_TA_full,d_n_bit);
m_sig_RB_full_ = oarf_get_frame(cardB);
m_sig_RB_full = m_sig_RB_full_(1:d_N_OFDM*(d_N_FFT+d_N_CP),5:8); 
m_sym_RB_full = f_ofdm_rx(m_sig_RB_full, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfB);


%% -------- SNR measurement -------- 
%** noise measurment **
%v_P_n = mean(var(squeeze(m_n_sym_RB),0,2));
v_P_n = mean(var(squeeze(m_n_sym_RB),0,2));
%** SNR caculation
%v_P_s_ideal = zeros(301,1);
%for d_f=1:d_N_f
%    v_H_A2B_ideal = squeeze(m_H_est_A2B(:,:,d_f));
%    v_P_s_ideal(d_f) = norm(v_H_A2B_ideal)^2;
%end
%keyboard;
v_P_s_ideal = var(squeeze(m_sym_RB_ideal),0,2);
v_P_s_iden = var(squeeze(m_sym_RB_iden),0,2);
v_P_s_diag = var(squeeze(m_sym_RB_diag),0,2);
v_P_s_full = var(squeeze(m_sym_RB_full),0,2);

v_SNR_ideal_ = 10*log10((v_P_s_ideal-v_P_n)/v_P_n);
v_SNR_iden_ = 10*log10((v_P_s_iden-v_P_n)/v_P_n);
v_SNR_diag_ = 10*log10((v_P_s_diag-v_P_n)/v_P_n);
v_SNR_full_ = 10*log10((v_P_s_full-v_P_n)/v_P_n);

v_SNR_ideal = nan(d_N_f+1,1);
v_SNR_iden = nan(d_N_f+1,1);
v_SNR_diag = nan(d_N_f+1,1) ;
v_SNR_full = nan(d_N_f+1,1) ;

v_SNR_ideal([1:151 153:302]) = v_SNR_ideal_([151:301 1:150]);
v_SNR_iden([1:151 153:302]) = v_SNR_iden_([151:301 1:150]) ;
v_SNR_diag([1:151 153:302]) = v_SNR_diag_([151:301 1:150]) ;
v_SNR_full([1:151 153:302]) = v_SNR_full_([151:301 1:150]);

save('-v7','result/bf_gain_4x1_t3.mat','v_SNR_ideal','v_SNR_iden','v_SNR_diag','v_SNR_full');
%% -------- plot --------
v_f = d_fc-floor(d_N_f/2)*d_delta_f:d_delta_f:d_fc+ceil(d_N_f/2)*d_delta_f; 
figure(5)
hold on
plot(v_f,v_SNR_ideal,'k-')
plot(v_f,v_SNR_iden,'m-.')
plot(v_f,v_SNR_diag,'r-')
plot(v_f,v_SNR_full,'b-')
hold off
ylim([30 40])
