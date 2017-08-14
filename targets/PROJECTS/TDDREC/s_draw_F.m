close all
%clear all
d_N_f = 301; 			% carrier number carrying data
d_N_meas = 10;

d_file_name = 'result/4a_l45_t10a.mat';
o_result = load(d_file_name);
m_H_B2A = o_result.m_H_B2A;
m_H_A2B = o_result.m_H_A2B;
d_N_loc = size(m_H_A2B,2)/d_N_meas;

m_F0 = zeros(size(m_H_B2A,1));
m_F1 = zeros(size(m_H_B2A,1));
m_F2 = zeros(size(m_H_B2A,1));

%% ** post processing **
% ** normalisation **
for d_l = 1:d_N_loc
    temp = (d_l-1)*d_N_meas+1:(d_l-1)*d_N_meas+d_N_meas;
    d_norm_fac = mean(mean(mean(abs(m_H_B2A(:,temp,:)))));
    m_H_B2A(:,temp,:) = m_H_B2A(:,temp,:)/d_norm_fac;
    m_H_A2B(:,temp,:) = m_H_A2B(:,temp,:)/d_norm_fac;
end

% ** average **
d_var_thr = 0.03;
m_H_A2B_ = [];
m_H_B2A_ = [];
for d_l = 1:d_N_loc
%     d_var_l = max(var(squeeze(m_H_A2B(:,(d_l-1)*d_N_meas+1:(d_l-1)*d_N_meas+d_N_meas,1)).'));
    d_var_l = max(var(m_H_A2B(:,(d_l-1)*d_N_meas+1:(d_l-1)*d_N_meas+d_N_meas,1).'));
    d_mean_l = mean(mean(abs(m_H_A2B(:,(d_l-1)*d_N_meas+1:(d_l-1)*d_N_meas+d_N_meas,1).')));
    if d_var_l/d_mean_l < d_var_thr
%      m_H_A2B_ = [m_H_A2B_ mean(m_H_A2B(:,(d_l-1)*d_N_meas+1:(d_l-1)*d_N_meas+d_N_meas,:),2)];
%      m_H_B2A_ = [m_H_B2A_ mean(m_H_B2A(:,(d_l-1)*d_N_meas+1:(d_l-1)*d_N_meas+d_N_meas,:),2)];
         m_H_A2B_ = [m_H_A2B_ m_H_A2B(:,(d_l-1)*d_N_meas+1:(d_l-1)*d_N_meas+d_N_meas,:)];
         m_H_B2A_ = [m_H_B2A_ m_H_B2A(:,(d_l-1)*d_N_meas+1:(d_l-1)*d_N_meas+d_N_meas,:)];
    end
end
%keyboard;
% % ** no processing **
% m_H_B2A_ = m_H_B2A;
% m_H_A2B_ = m_H_A2B;

%% ** calibration **
% temp = 1:15;
for d_f = 1:d_N_f     
     [m_F0(:,:,d_f),m_A0_est,m_B0_est] = f_tls_svd(m_H_B2A_(:,:,d_f).',m_H_A2B_(:,:,d_f).');    
%    [m_F0(:,:,d_f),m_A0_est,m_B0_est,d_err0] = f_tls_svd(m_H_B2A_(:,temp,d_f).',m_H_A2B_(:,temp,d_f).');    
%    [m_F1(:,:,d_f),m_A1_est,m_B1_est d_err1] = f_tls_ap(m_H_B2A_(:,:,d_f).',m_H_A2B_(:,:,d_f).'); 
    [m_F2(:,:,d_f),m_A2_est,m_B2_est] = f_tls_diag(m_H_B2A_(:,:,d_f).',m_H_A2B_(:,:,d_f).');
end

save('-v7','-append',d_file_name,'m_F0','m_F2');

%% ** plot **
h10 = figure(10);
hold on;
for d_f=1:d_N_f
  m_F= m_F0(:,:,d_f);
  plot(diag(m_F),'bo')
  plot(diag(m_F,1),'k+')
  plot(diag(m_F,-1),'y+')
  plot(diag(m_F,2),'rx')
  plot(diag(m_F,-2),'mx')
   plot(diag(m_F,3),'g*')
   plot(diag(m_F,-3),'c*')
end
hold off;
title('F0');
% axis([-0.1 0.1 -0.1 0.1])
axis([-2 2 -2 2])
% axis([-0.3 0.3 -0.3 0.3])
grid on
% saveas(h10,'results\F_svd_2c_g20_0_t1','fig');
% print('-depsc','results\F_svd_2c_g20_0_t1.eps')
% 
% h11=figure(11);
% hold on;
% for d_f=1:d_N_f
%   m_F= m_F1(:,:,d_f);
%   plot(diag(m_F),'bo')
%   plot(diag(m_F,1),'k+')
%   plot(diag(m_F,-1),'y+')
%   plot(diag(m_F,2),'rx')
%   plot(diag(m_F,-2),'mx')
%   plot(diag(m_F,3),'g*')
%   plot(diag(m_F,-3),'c*')
% end
% hold off;
% axis([-0.3 0.3 -0.3 0.3])
% % axis([-2 2 -2 2])
% grid on
% saveas(h11,'results\F_ap_2c_g20_0_t2','fig');
% print('-depsc','results\F_ap_2c_g20_0_t2.eps')
% 
% h12=figure(12);
% hold on;
% for d_f=1:d_N_f
%   m_F= m_F2(:,:,d_f);
%   plot(diag(m_F),'bo')
% %   plot(diag(m_F,1),'k+')
% %   plot(diag(m_F,-1),'k+')
% %   plot(diag(m_F,2),'rx')
% %   plot(diag(m_F,-2),'rx')
% %   plot(diag(m_F,3),'g*')
% %   plot(diag(m_F,-3),'g*')
% end
% hold off;
% title('F2');
% axis([-2 2 -2 2])
% grid on
% saveas(h12,'results\F_diag_2c_g20_0_t2','fig');
% print('-depsc','results\F_diag_2c_g20_0_t2.eps')

