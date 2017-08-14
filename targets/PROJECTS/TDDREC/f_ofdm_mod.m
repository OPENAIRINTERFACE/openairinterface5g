function m_sig_T = f_ofdm_mod(m_sym_T, d_N_FFT, d_N_CP, d_N_OFDM, v_active_rf, d_amp)

d_N_ant_act = sum(v_active_rf);

%** mapping useful data to favorable carriers **
m_sym_T_ext = zeros(d_N_FFT,d_N_OFDM,d_N_ant_act);
m_sym_T_ext(2:151,:,:) = m_sym_T(1:150,:,:);
m_sym_T_ext(362:512,:,:) = m_sym_T(151:301,:,:);

%** ifft **
m_sig_T_ = sqrt(d_N_FFT)*ifft(m_sym_T_ext,d_N_FFT,1);

%** add cyclic prefix **
m_sig_T_ = [m_sig_T_(end-d_N_CP+1:end,:,:); m_sig_T_];
d_L = (d_N_FFT+d_N_CP)*d_N_OFDM;
m_sig_T = floor(reshape(m_sig_T_,d_L,d_N_ant_act)*d_amp);

end
