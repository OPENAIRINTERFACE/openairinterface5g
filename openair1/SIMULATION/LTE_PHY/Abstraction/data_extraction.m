% close all
% clear all

global SINR_p BLER_meas snr bler SINR_awgn abs_mode modu Pm

set(0, 'DefaultLineMarkerSize', 10);
set(0, 'Defaultaxesfontsize', 10);
set(0, 'DefaultLineLineWidth', 2);
set(0, 'DefaultAxesFontName', 'Helvetica');
set(0, 'DefaultTextFontName', 'Helvetica');

% plot_style = {'b*';'r*';'g*';'y*';'k*';...
%     'bo';'ro';'go';'yo';'ko';...
%     'bs';'rs';'gs';'ys';'ks';...
%     'bd';'rd';'gd';'yd';'kd';...
%     'bx';'rx';'gx';'yx';'kx';...
%     'b+';'r+';'g+';'y+';'k+'};

echo off;
 load '/homes/latif/devel/trunk/openair1/SIMULATION/LTE_PHY/ideal_dataout_tx1.mat'
% load '/extras/kaltenbe/L2S_map_results/Perfect_CE_MIESM/Mat_Files/siso_MI_16Qam.mat'
% load '/extras/kaltenbe/L2S_map_results/Perfect_CE_MIESM/Mat_Files/siso_MI_64Qam.mat'
%load '/extras/kaltenbe/L2S_map_results/Real_CE_EESM/matfiles/mcs_channel.mat';
%pathname = '/extras/kaltenbe/L2S_map_results/Real_CE_EESM/results_scmc/';
%root_path = '/homes/latif/OpenAirPlatform/test_devel/openair1_multi_eNb_UE/SIMULATION/LTE_PHY/BLER_SIMULATIONS/awgn_bler_new/';
%%
mcs = [4 7 9 13 16 19 22 24];
s2=[];
s3=[];
abs_mode=0;
Pm=100;
modu=4; %2*50
for m=1:1:length(mcs)
    if(mcs(m) >9 && mcs(m) < 17)
        modu=16; 
        Pm=200; %4*50
    else if(mcs(m) > 16 && mcs(m) < 28)
            modu = 64;
            Pm=300; %6 *50
        end
    end
    
    %% SINR_P Extraction per Sub carrier basis
    eval(['data_all = data_all' num2str(mcs(m)) ';']);
     data_all = data_all(data_all(:,end)<0.9,:);
     data_all = data_all(data_all(:,end)>0.01,:);
    %
    SINR_p = data_all(:,2:51);
    data = dlmread(sprintf('/homes/latif/devel/trunk/openair1/SIMULATION/LTE_PHY/BLER_SIMULATIONS/AWGN_SISO_IDEAL_03082011/ideal_ce_bler_tx1_mcs%d.csv',mcs(m)),';',1,0);
    snr = data(:,1);
    bler = data(:,5)./data(:,6); % round 1
    BLER_meas = data_all(:,end);
    p=50;

    %% for coarse beta calculation:
    beta_vec = 0:1:30;
    delta = zeros(size(beta_vec));
    for beta=1:length(beta_vec)
        delta(beta)=delta_BLER(beta_vec(beta));
    end
    [val ind] = min(delta);
    opt_beta_new(mcs(m)) = beta_vec(ind);
    %% for optmimized beta calculation
    opt_beta(mcs(m)) = fminsearch(@delta_BLER,opt_beta_new(mcs(m)));

      %% MIESM Mapping
%     if(mcs(i) >=1 || mcs(i) < 10)
%         load '/extras/kaltenbe/L2S_map_results/Perfect_CE_MIESM/Mat_Files/siso_MI_4Qam.mat'
%         sum_m = 600; %2*300 4QAM
%     else if(mcs(i) >=10 || mcs(i) < 17)
%             load '/extras/kaltenbe/L2S_map_results/Perfect_CE_MIESM/Mat_Files/siso_MI_16Qam.mat'
%             sum_m = 1200; %4*300 16QAM 
%         else
%             load '/extras/kaltenbe/L2S_map_results/Perfect_CE_MIESM/Mat_Files/siso_MI_64Qam.mat'
%             sum_m = 1800; %6*300 64QAM
%         end
%     end
%     %data = dlmread(fullfile(root_path,sprintf('scmc_bler_%d.csv',mcs(m))),';',1,0);
%   %data = dlmread(fullfile(root_path,sprintf('bler_tx1_mcs%d.csv',mcs(m))),';',1,0);
if(abs_mode==0)
    
    eval(['load ' '/homes/latif/devel/trunk/openair1/SIMULATION/LTE_PHY/siso_MI_abs_' num2str(modu) 'Qam.mat'])
    [x y]= size(SINR_p);
    RBIR = [];
    for t=1:1:x
        s = SINR_p(t,:);
        eval(['SI_p = interp1(newSNR,newC_siso_' num2str(modu) 'QAM,s, ''linear'' , ''extrap'');']);
        %SI_p = interp1(newSNR,newC_siso_4QAM,s, 'linear','extrap');
        RBIR(t) = sum(SI_p/opt_beta(mcs(m)))/Pm;
    end
    SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear');
else
    SINR_eff= 10*log10(-1*opt_beta(mcs(m))*log((1/p)*sum(exp((-10.^(SINR_p'./10))./opt_beta(mcs(m))))));
end

    
    
  %% EESM Mapping
%     p=50;
%     %data = dlmread(fullfile(root_path,sprintf('scmc_bler_%d.csv',mcs(m))),';',1,0);
%     %data = dlmread(fullfile(root_path,sprintf('bler_tx1_mcs%d.csv',mcs(m))),';',1,0);
%     data = dlmread(sprintf('ideal_ce_bler_tx1_mcs%d.csv',mcs(m)),';',1,0);
%     snr = data(:,1);
%     bler = data(:,5)./data(:,6); % round 1
%     BLER_meas = data_all(:,end);
%     
%     %% SNR Method
%     % SINR_awgn = interp1(bler, snr, BLER_meas,'cubic');
%     % SINR_awgn = SINR_awgn';
%     
%     %% for coarse beta calculation:
%     beta_vec = 0:1:30;
%     delta = zeros(size(beta_vec));
%     for beta=1:length(beta_vec)
%         delta(beta)=delta_BLER(beta_vec(beta));
%     end
%     [val ind] = min(delta);
%     opt_beta_new(mcs(m)) = beta_vec(ind);
%     %% for optmimized beta calculation
%     opt_beta(mcs(m)) = fminsearch(@delta_BLER,opt_beta_new(mcs(m)));
    
    %% BLER Plot w.r.t Effective SINR
    %opt_beta(mcs(m)) = 1.125;
    %SINR_eff= 10*log10(-1*opt_beta(mcs(m))*log((1/p)*sum(exp((-10.^(SINR_p'./10))./opt_beta(mcs(m))))));
    %SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear');  
    h_fig = figure(m);
    hold off
    semilogy(SINR_eff,BLER_meas,'rd')
    hold on
    grid on
    semilogy(snr,bler,'y-*');
    xlim([-10 20])
    ylim([1e-3 1])
    s = strcat('L2S Map using EESM for LTE MCS ', num2str(mcs(m)), ' and beta= ',num2str(opt_beta(mcs(m))));
    title(s);
    ylabel 'BLER'
    xlabel 'SINR_{effective}'
    s2 = strcat('BLER_{meas} MCS ', num2str(mcs(m)));
    s3 = strcat('BLER_{AWGN} MCS ', num2str(mcs(m)));
    legend(s2,s3, 'Location',  'Best');
%      saveas(h_fig,fullfile(pathname,strcat('mcs',num2str(mcs(m)) ,'.eps')),'epsc2');
%      saveas(h_fig,fullfile(pathname,strcat('mcs',num2str(mcs(m)) ,'.fig')),'fig');
%      saveas(h_fig,fullfile(pathname,strcat('mcs',num2str(mcs(m)) ,'.jpg')),'jpg');
    
end
%% Saving adjustment factor (beta)
% save(fullfile(pathname, 'opt_beta.dat'),'opt_beta');
