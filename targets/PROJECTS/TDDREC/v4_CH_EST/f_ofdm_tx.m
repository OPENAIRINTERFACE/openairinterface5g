%
% PURPOSE : OFDM Transmitter
%
% ARGUMENTS :
%
% d_M 			: modulation order
% d_N_f			: carrier number carrying data
% d_N_FFT		: total carrier number
% d_N_CP		: extented cyclic prefix
% d_N_OFDM		: OFDM symbol number per frame
% v_active_rf	: active RF antenna indicator
% d_amp         : amplitude
%
% OUTPUTS :
%
% m_sym_T 		: transmitted signal before IFFT with dimension d_N_f x d_N_OFDM x d_N_ant_act
% m_sig_T		: OFDM signal with dimension ((d_N_FFT+d_N_CP)*d_N_OFDM) x d_N_ant
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
function [m_sym_T, m_sig_T] = f_ofdm_tx(d_M, d_N_f, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rf, d_amp)

d_N_ant_act = sum(v_active_rf);

%** constellation table **
v_MPSK = exp(sqrt(-1)*([1:d_M]*2*pi/d_M+pi/d_M));								
%** transmitted symbol **
%v_state = [1;2;3;4];
%rand("state",v_state);
m_sym_T = v_MPSK(ceil(rand(d_N_f, d_N_OFDM, d_N_ant_act)*d_M));   

%** mapping useful data to favorable carriers **
m_sym_T_ext = zeros(d_N_FFT,d_N_OFDM,d_N_ant_act);
m_sym_T_ext(2:151,:,:) = m_sym_T(151:300,:,:);
m_sym_T_ext(363:512,:,:) = m_sym_T(1:150,:,:);

%** ifft **
m_sig_T_ = sqrt(d_N_FFT)*ifft(m_sym_T_ext,d_N_FFT,1);

%** add cyclic prefix **
m_sig_T_ = [m_sig_T_(end-d_N_CP+1:end,:,:); m_sig_T_];
d_L = (d_N_FFT+d_N_CP)*d_N_OFDM;
m_sig_T = floor(reshape(m_sig_T_,d_L,d_N_ant_act)*d_amp);

end
