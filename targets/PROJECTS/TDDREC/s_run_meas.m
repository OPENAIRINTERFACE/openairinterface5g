%
% SCRIPT ID : s_run_meas
%
% PROJECT NAME : TDD Recoprocity
%
% PURPOSE : full transmission and receive train for TDD reciprocity calibration
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
% Apr-29-2014  X. JIANG       0.1     script creation v0.1
%
%  REFERENCES/NOTES/COMMENTS :
%
% - Based on the script "run_full_duplex" created by Mirsad Cirkic, Florian Kaltenberger.
% 
%**********************************************************************************************

%% ** initialisation **
%% ------------- to change in experiment ------------
%clc
close all
%clear all
% 
%% ----------------------------------------------------
m_sym_TA = zeros(d_N_f,d_N_OFDM,length(v_indA),d_N_meas);
m_sym_TB = zeros(d_N_f,d_N_OFDM,length(v_indB),d_N_meas);
m_sym_RA = zeros(d_N_f,d_N_OFDM,length(v_indA),d_N_meas);
m_sym_RB = zeros(d_N_f,d_N_OFDM,length(v_indB),d_N_meas);
m_sym_TA_ = zeros(d_N_f,d_N_OFDM,length(v_indA));
m_sig_TA = zeros((d_N_CP+d_N_FFT)*d_N_OFDM*2,d_N_antA);
m_sig_TB = zeros((d_N_CP+d_N_FFT)*d_N_OFDM*2,d_N_antB);

for d_n_meas = 1:d_N_meas
    %% -------- tx -------- 
    %** tx of site A **
    [m_sym_TA_(:,:,:,d_n_meas), m_sig_TA_] = f_ofdm_tx(d_M, d_N_f, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfA, d_amp);
    
    for d_a = 1:d_N_antA_act
	d_N_a = d_N_OFDM/d_N_antA_act;
	d_N_b = d_N_OFDM*(d_N_FFT+d_N_CP)/d_N_antA_act;
	m_sig_TA(d_N_b*(d_a-1)+1:d_N_b*d_a,v_indA(d_a)) = m_sig_TA_(d_N_b*(d_a-1)+1:d_N_b*d_a,d_a);    
	m_sym_TA(:,d_N_a*(d_a-1)+1:d_N_a*d_a,d_a,d_n_meas) = m_sym_TA_(:,d_N_a*(d_a-1)+1:d_N_a*d_a,d_a,d_n_meas);
    end

    m_sig_TA(1:end/2,v_indA)= m_sig_TA(1:end/2,v_indA)*2;    %affect the LSB to 0 to set on Tx mode
    m_sig_TA(end/2+1:end,v_indA)= 1+1i;           %affect the LSB to 1 to set on Rx mode
    
    %** tx of site B **
    [m_sym_TB(:,:,:,d_n_meas), m_sig_TB_] = f_ofdm_tx(d_M, d_N_f, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfB, d_amp);
    m_sig_TB(1:end/2,v_indB)=1+1i;
    m_sig_TB(end/2+1:end,v_indB)= m_sig_TB_*2;    

    
    %** Transmission from A to B **
    oarf_send_frame(cardA,m_sig_TA,d_n_bit);
    %** Transmission from B to A **
    oarf_send_frame(cardB,m_sig_TB,d_n_bit);
    %sleep(0.1);
    
    m_sig_R = oarf_get_frame(-2);
    m_sig_RA = m_sig_R(d_N_OFDM*(d_N_FFT+d_N_CP)+1:d_N_OFDM*(d_N_FFT+d_N_CP)*2,1:d_N_antA);
    m_sig_RB = m_sig_R(1:d_N_OFDM*(d_N_FFT+d_N_CP),d_N_antA+1:d_N_antA+d_N_antB);
    %% -------- rx --------  
    m_sym_RB(:,:,:,d_n_meas) = f_ofdm_rx(m_sig_RB, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfB);
    m_sym_RA(:,:,:,d_n_meas) = f_ofdm_rx(m_sig_RA, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rfA);
   %keyboard;
end

%** channel estimation **
m_H_est_A2B = f_ch_est(m_sym_TA, m_sym_RB);           %dimension: d_N_antR x d_N_antT x d_N_f x d_N_meas
m_H_est_B2A = f_ch_est(m_sym_TB, m_sym_RA);

%% -------- plot --------

%** channel estimation in frequency domain **
m_H_A2B_draw = squeeze(m_H_est_A2B(1,:,:,end)).';
m_H_B2A_draw = squeeze(m_H_est_B2A(:,1,:,end)).';

figure(1)
subplot(2,1,1)
plot(real(m_sig_RB(1:40,v_indB)),'b-');
hold on
plot(real(m_sig_RB(end-80+1:end-40,v_indB)),'g-')
hold on
plot(real(m_sig_RB(end-40+1:end,v_indB)),'r-')
title('m_sig_RB')
subplot(2,1,2)
plot(real(m_sig_RA(:,v_indA)),'-');
title('m_sig_RA')

figure(2)
subplot(2,2,1)
plot(20*log10(abs(m_H_A2B_draw)),'-');
title('|h| vs. freq (A2B)')
xlabel('freq')
ylabel('|h|')
ylim([0 100])

subplot(2,2,2)
plot(20*log10(abs(m_H_B2A_draw)),'-');
title('|h| vs. freq (B2A)')
xlabel('freq')
ylabel('|h|')
ylim([0 100])

subplot(2,2,3)
plot(angle(m_H_A2B_draw),'-');
title('angle(h) vs. freq (A2B)')
xlabel('freq')
ylabel('angle(h)')

subplot(2,2,4)
plot(angle(m_H_B2A_draw),'-');
title('angle(h) vs. freq (B2A)')
xlabel('freq')
ylabel('angle(h)')

%v_color = ['b*','g*','r*','c*'];
figure(3)
for d_a = 1:d_N_antA_act
    subplot(2,2,d_a);
    plot(m_sym_RA(1,:,d_a,1),'*');
    title(strcat('m sym RA',num2str(d_a)));
end

figure(4)
for d_a = 1:d_N_antA_act
    subplot(2,2,d_a);
    plot(m_sym_RB(1,d_N_OFDM/d_N_antA_act*(d_a-1)+1:d_N_OFDM/d_N_antA_act*d_a,1,1),'*');
    hold on;
    title(strcat('m sym RB',num2str(d_a)));
end
