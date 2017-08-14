%
% PURPOSE : OFDM Receiver
%
% ARGUMENTS :
%
% m_sig_R		: received signal with dimension ((d_N_FFT+d_N_CP)*d_N_ofdm) x d_N
% d_N_FFT		: total carrier number
% d_N_CP		: extented cyclic prefix
% d_N_OFDM		: OFDM symbol number per frame
% v_active_rf	: active RF antenna indicator
%
% OUTPUTS :
%
% m_sym_R 		: transmitted signal before IFFT with dimension d_N_f x d_N_ofdm x d_N_ant_act
%
%**********************************************************************************************
%                            EURECOM -  All rights reserved
%
% AUTHOR : Xiwen JIANG, Florian Kaltenberger
%
% DEVELOPMENT HISTORY :
% 
% Date         Name(s)       Version  Description
% -----------  ------------- -------  ------------------------------------------------------
% Apr-29-2014  X. JIANG       0.1     creation of code
%
% REFERENCES/NOTES/COMMENTS :
%
% - Based on the function "genrandpskseq" created by Mirsad Cirkic, Florian Kaltenberger.
% 
%**********************************************************************************************
function m_sym_R = f_ofdm_rx(m_sig_R, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rf)

d_N_ant_act = sum(v_active_rf);
m_sig_R_eff = m_sig_R(:,find(v_active_rf));
m_sig_R_f = reshape(m_sig_R_eff,(d_N_FFT+d_N_CP),d_N_OFDM,d_N_ant_act);

%** delete the CP **
m_sig_R_noCP = m_sig_R_f(d_N_CP+1:end,:,:);

%** fft **
m_sym_R_fft = 1/sqrt(d_N_FFT)*fft(m_sig_R_noCP,d_N_FFT,1);
%m_sym_R_fft = fft(m_sig_R_noCP,d_N_FFT,1);
m_sym_R = m_sym_R_fft([363:512 2:151],:,:);

end
