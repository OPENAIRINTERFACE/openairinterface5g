%
% PURPOSE : channel estimation using least square method
%
% ARGUMENTS :
% 
% m_sym_T		: transmitted symbol, d_N_f x d_N_ofdm x d_N_ant_act x d_N_meas
% m_sym_R		: received symbol, d_N_f x d_N_ofdm x d_N_ant_act x d_N_meas
% d_N_meas      : number of measurements
% 
% OUTPUTS :
%
% m_H_est 		: estimation of sub-channels, d_N_antR x d_N_antT x d_N_f x d_N_meas
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
% - Based on the function "runmeas_full_duplex" created by Mirsad Cirkic, Florian Kaltenberger.
% 
%**********************************************************************************************
function m_H_est = f_ch_est(m_sym_T, m_sym_R)

%% ** initialisation **
[d_N_f,d_N_OFDM,d_N_antT,d_N_meas] = size(m_sym_T);
d_N_antR = size(m_sym_R,3);
m_H_est = zeros(d_N_antR,d_N_antT,d_N_f,d_N_meas);

%% ** estimate the subband channel for each measurement and antenna **
for d_n_meas = 1:d_N_meas
    for d_n_f = 1:d_N_f
        m_y = reshape(m_sym_R(d_n_f,:,:,d_n_meas),d_N_OFDM,d_N_antR).';        % squeeze: problem for antenna number = 1 case
        m_s = reshape(m_sym_T(d_n_f,:,:,d_n_meas),d_N_OFDM,d_N_antT).';
        m_H_est(:,:,d_n_f,d_n_meas) = m_y*m_s'/(m_s*m_s');                   % LS channel estimation
    end
end

end
