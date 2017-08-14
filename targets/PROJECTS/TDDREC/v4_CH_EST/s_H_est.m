%
% SCRIPT ID : s_H_est
%
% PROJECT NAME : TDD Recoprocity
%
% PURPOSE : full transmission and receive train for calibration channel estimation
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
% May-11-2015  X. JIANG       0.1     script creation v0.1
%
%  REFERENCES/NOTES/COMMENTS :
%
%**********************************************************************************************

%% ** initialisation **
clc
close all
clear all
s_init_params;
 
d_M = 4;				% modulation order, e.g. 4 means QPSK

%%** frequency **
d_N_f = 300; 			% carrier number carrying data
d_N_FFT = 512;			% total carrier number
d_N_CP = 128;			% extented cyclic prefix
%%** time **
d_N_OFDM = 120;			% number of ofdm symbol per frame
d_N_meas = 10;			% measuement number
%%** antenna **
d_N_antA = 4;			% antenna number at site a
d_N_antB = 4; 			% antenna number at site b
v_indA = find(v_active_rfA);	% active antenna index at site a
v_indB = find(v_active_rfB);	% active antenna index at site b
d_amp = pow2(12);
%% ----------------------------------------------------
m_sym_TA = zeros(d_N_f,d_N_OFDM,length(v_indA),d_N_meas);
m_sym_TB = zeros(d_N_f,d_N_OFDM,length(v_indA),d_N_meas);
m_sym_RB = zeros(d_N_f,d_N_OFDM,length(v_indB),d_N_meas);
m_sig_TA = zeros((d_N_CP+d_N_FFT)*d_N_OFDM,d_N_antA);
m_sig_TB = zeros((d_N_CP+d_N_FFT)*d_N_OFDM,d_N_antB);

%** simulation**

for d_n_meas = 1:d_N_meas
    %% -------- tx -------- 
    %** tx of site A **
    [m_sym_TA(:,:,:,d_n_meas), m_sig_TA_] = f_ofdm_tx(d_M, d_N_f, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfA, d_amp);
    
    %% -------- channel --------     
    %** Transmission from A to B **
    m_sig_TA(:,v_indA)= m_sig_TA_*2;                        %affect the LSB to 0 to set on Tx mode 
    m_sig_TB(:,v_indB)= 1+1i*ones(size(m_sig_TB,1),length(v_indB));  %affect the LSB to 1 to set on Rx mode
    oarf_send_frame(cardB,m_sig_TB,d_n_bit);
    oarf_send_frame(cardA,m_sig_TA,d_n_bit);
    m_sig_R = oarf_get_frame(cardA);
    m_sig_RB = m_sig_R(1:d_N_OFDM*(d_N_FFT+d_N_CP),d_N_antA+1:d_N_antA+d_N_antB);
    sleep(0.1);
    %% -------- rx --------  
    m_sym_RB(:,:,:,d_n_meas) = f_ofdm_rx(m_sig_RB, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfB);

end
%keyboard;

%** channel estimation **
m_H_est_A2B_f = f_ch_est(m_sym_TA, m_sym_RB);           %dimension: d_N_antR x d_N_antT x d_N_f x d_N_meas
m_H_est_A2B_ = zeros(length(v_indB),length(v_indA),d_N_FFT,d_N_meas);
m_H_est_A2B_(:,:,[2:151 363:512],:) = m_H_est_A2B_f(:,:,[151:300 1:150],:);
m_H_est_A2B_t = ifft(m_H_est_A2B_,[],3);

%% -------- plot --------

%** channel estimation in frequency domain **
 m_H_A2B_draw_f = squeeze(m_H_est_A2B_f(1,:,:,1)).';
 m_H_A2B_draw_t = squeeze(m_H_est_A2B_t(1,:,:,1)).';
 
 figure(1)
 plot(real(m_sig_RB(1:1000,v_indB)),'b-');
 title('received signal at site B (time domain)')

 figure(2)
 plot(m_sym_RB(100,:,1,1),'*')
 title('Demodulated symboles at site B')

 figure(3)
 plot(abs(m_H_A2B_draw_t))
 title('Impulse Response')

 figure(4)
 subplot(2,1,1)
 plot(20*log10(abs(m_H_A2B_draw_f)),'-');
 title('|h| vs. freq (A2B)')
 xlabel('freq')
 ylabel('|h|')
 ylim([0 100])
 
 subplot(2,1,2)
 plot(angle(m_H_A2B_draw_f),'-');
 title('angle(h) vs. freq (A2B)')
 xlabel('freq')
 ylabel('angle(h)')
 
