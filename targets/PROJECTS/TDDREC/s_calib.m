%
% SCRIPT ID : s_run_calib
%
% PROJECT NAME : TDD Recoprocity
%
% PURPOSE : channel calibration for MISO case
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
% - Based on the script "calibration" created by Mirsad Cirkic, Florian Kaltenberger.
% 
%**********************************************************************************************

%% ** initilisation **
%---------- to change in experiement --------- 
%s_init_params;
%clc
%clear all
close all
%d_N_f = 301;  	        % carrier number carrying data
%d_N_meas = 10;          % measuement number
%d_N_loc = 5;            % Rx locations
%d_N_antM = 2;           % max active antenna number for site a and site b
%----------------------------------------------

%% -------- System parameters --------
d_M = 4;				% modulation order, e.g. 4 means QPSK

%** frequency **
d_N_f = 301; 			% carrier number carrying data
d_N_FFT = 512;			% total carrier number
d_N_CP = 128;			% extented cyclic prefix
%** time **
d_N_OFDM = 60;			% number of ofdm symbol per half frame
d_N_meas = 10;			% measuement number
%** space **
d_N_antA = 4;			% antenna number at site a
d_N_antB = 4; 			% antenna number at site b
v_indA = find(v_active_rfA);	% active antenna index at site a
v_indB = find(v_active_rfB);	% active antenna index at site b
d_N_antA_act = length(v_indA);
d_N_antB_act = length(v_indB);

%** amplitude **
d_amp = pow2(12.5)-1;   % to see how to be used??

%% -------- calibration parameters -------
d_N_loc = 45;            % Rx locations
d_N_antM = max(sum(v_active_rfA),sum(v_active_rfB));          % max active antenna number for site a and site b

%% -------- initialisation ----------------
m_H_A2B = zeros(d_N_antM,d_N_meas*d_N_loc, d_N_f);           % d_N_antA x (d_N_meas*d_N_loc) x d_N_f
m_H_B2A = zeros(d_N_antM,d_N_meas*d_N_loc, d_N_f);           % d_N_antA x (d_N_meas*d_N_loc) x d_N_f

m_F0 = zeros(d_N_antM,d_N_antM,d_N_f);
m_F1 = zeros(d_N_antM,d_N_antM,d_N_f);

%% ** collect the measurement data from different locations **
for d_loc = 1:d_N_loc
    % run measurement, note: uncomment "clear all"
    s_run_meas;                                   
    % --- the following part is dedicated to A2B MISO -----
    m_H_A2Bi = permute(squeeze(m_H_est_A2B),[1 3 2]);
    m_H_B2Ai = permute(squeeze(m_H_est_B2A),[1 3 2]);
    % -----------------------------------------------------
    
    m_H_A2B(:,(d_loc-1)*d_N_meas+1:d_loc*d_N_meas,:) = m_H_A2Bi;
    m_H_B2A(:,(d_loc-1)*d_N_meas+1:d_loc*d_N_meas,:) = m_H_B2Ai;
    %keyboard;
    disp('Please move the antenna to another location and press any key to continue');
%    pause
    pause(5);
end
save('-v7','result/4a_l45_t10d.mat','m_H_A2B','m_H_B2A'); 
%save('-v7','result/2c_l15_t2a.mat','m_H_A2B','m_H_B2A'); 

%% ** calibration **
for d_f = 1:d_N_f
    [m_F0(:,:,d_f),m_A0_est,m_B0_est] = f_tls_svd(m_H_B2A(:,:,d_f).',m_H_A2B(:,:,d_f).');       % solve the TLS problem using SVD method
    [m_F1(:,:,d_f),m_A1_est,m_B0_est] = f_tls_ap(m_H_B2A(:,:,d_f).',m_H_A2B(:,:,d_f).');        % solve the TLS problem using Alternative Projection method
end

%% ** plot **
figure(10)
hold on;
for d_f=1:size(m_F0,3);
  m_F= m_F0(:,:,d_f);
  plot(diag(m_F),'bo')
  plot(diag(m_F,1),'k+')
  plot(diag(m_F,2),'rx')
  plot(diag(m_F,3),'g*')
  plot(diag(m_F,-1),'y+')
  plot(diag(m_F,-2),'mx')
  plot(diag(m_F,-3),'c*')
end
hold off;
title('F0');
axis([-2 2 -2 2])
grid on

figure(11)
hold on;
for d_f=1:size(m_F1,3);
  m_F= m_F1(:,:,d_f);
  plot(diag(m_F),'bo')
  plot(diag(m_F,1),'k+')
  plot(diag(m_F,2),'rx')
  plot(diag(m_F,3),'g*')
  plot(diag(m_F,-1),'y+')
  plot(diag(m_F,-2),'mx')
  plot(diag(m_F,-3),'c*')
end
hold off;
title('F1');
axis([-2 2 -2 2])
grid on
