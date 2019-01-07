%***************************************************************************************
% Calibration of Adjustment Factors for Exponential Effective SINR Mapping Methdology
% For 3GPP Long Term Evolution (LTE)
% Eurecom Institute
% (c) Imran Latif, Communications Mobile, 2012
%***************************************************************************************
%close all
%clear all

%***************************************************************************************
% Some basic initializations
%***************************************************************************************
global SINR_p BLER_meas abs_mode modu Pm tx_mode alpha seta
warning on
set(0, 'DefaultLineMarkerSize', 10);
set(0, 'Defaultaxesfontsize', 10);
set(0, 'DefaultLineLineWidth', 2);
set(0, 'DefaultAxesFontName', 'Helvetica');
set(0, 'DefaultTextFontName', 'Helvetica');

plot_style = {'b*';'r*';'g*';'y*';'k*';...
    'bo';'ro';'go';'yo';'ko';...
    'bs';'rs';'gs';'ys';'ks';...
    'bd';'rd';'gd';'yd';'kd';...
    'bx';'rx';'gx';'yx';'kx';...
    'b+';'r+';'g+';'y+';'k+'};

echo off;
pathname = '/homes/latif/devel/trunk/openair1/SIMULATION/LTE_PHY/TM5_ideal_CE/MIESM/M1/Polyfit';
%root_path = '/homes/latif/OpenAirPlatform/test_devel/openair1_multi_eNb_UE/SIMULATION/LTE_PHY/BLER_SIMULATIONS/awgn_bler_new/';
%%
%***************************************************************************************
% Input Parameters used for the training of adjustment Factors (betas)
%***************************************************************************************
mcs = [0 2 4 6 7 8 9];
% An array which hold the number of LTE MCS which is to be calibrated for
% abstraction

s2=[];
s3=[];
% String arrays used for printing in plots

tx_mode=5;
%1--> SISO, 2 --> Alamouti, 5 --> MU-MIMO ,6 --> TX Beamforming
% tells which transmission mode is to be abstracted.

abs_mode=2; % 0:MIESM (M2-MIESM), 1:EESM, 2:IA-MIESM (M1-MIESM)
%tells which abstraction methodology is to be used.

Pm=100; %2*50 QPSK
% Used for calculating RBIR

modu=4;
% Modulation used.

p=50;
save_out=1;


if(save_out)
    fid = fopen('IA_polyfit_M1_MIESM.tex','w');
    fprintf(fid,'\\documentclass[english,12pt,a4paper]{article}\n\n');
    fprintf(fid,'\\usepackage{times}\n');
    fprintf(fid,'\\usepackage{babel}\n');
    fprintf(fid,'\\usepackage{graphicx}\n');
    fprintf(fid,'\\usepackage{hyperref}\n\n');
    fprintf(fid,'\\begin{document}\n\n');
    fprintf(fid,'\\title{%s}\n\n','MU-MIMO with IA: Results of Abstraction using Polynomial fitting for M1-MIESM');
    fprintf(fid,'\\maketitle\n\n');
    fprintf(fid,'\\newpage\n\n');
    fprintf(fid,'\\listoffigures\n\n');
    fprintf(fid,'\\listoftables\n\n');
    fprintf(fid,'\\newpage\n\n');
end

disp('*******************************************************************\n')
%printf('TX Mode = %d, Abstraction Mode = %d\n',tx_mode,abs_mode);
disp('*******************************************************************\n')
%***************************************************************************************
% Main Execution Loop starts from here:
%***************************************************************************************
for m=1:1:length(mcs)
    
    ss = strcat('LTE TM5 M1 MIESM MCS ', num2str(mcs(m)));
    
    if(mcs(m) >9 && mcs(m) < 17)
        modu=16;
        Pm=200; %4*50 16QAM
    else if(mcs(m) > 16 && mcs(m) < 28)
            modu = 64;
            Pm=300; %6 *50  64QAM
        end
    end
    %%
    % %***************************************************************************************
    % %Reference AWGN Curves
    % %***************************************************************************************
    %     data = dlmread(sprintf('/homes/latif/devel/trunk/openair1/SIMULATION/LTE_PHY/awgn_bler_tx1_mcs%d.csv',mcs(m)),';',1,0);
    %     snr = data(:,1);
    %     ble = data(:,5)./data(:,6); % round 1
    %
    %    blerr = ble(ble<0.99);
    %    bler = blerr(blerr>0.001);
    %
    %    snr = snr(ble<0.99);
    %    snr = snr(blerr>0.001);
    %
    % %     snr_interp = snr(1):0.001:snr(end);
    % %     bler_interp = interp1(snr,bler,snr_interp,'linear');
    % %     snr = snr_interp;
    % %     bler = bler_interp;
    %
    % %***************************************************************************************
    % % Data Extraction on Sub carrier basis data file saved using dlsim and reference
    % % SINR Calculation based on Reference AWGN Curve
    % %***************************************************************************************
    eval(['data_all = data_all' num2str(mcs(m)) ';']);
    BLER_meas = data_all(:,end);
    %
    %     SINR_awgn = interp1(bler, snr, BLER_meas,'linear','extrap');
    %     SINR_awgn = SINR_awgn';
    %%
    %***************************************************************************************
    % Calculation of SINR per subcarrier
    %***************************************************************************************
    %for SISO
    switch(tx_mode)
        case 1
            ch_tmp = data_all(:,2:51);
            abs_channel = (abs(ch_tmp).^2);
            input_snr = 10.^(data_all(:,1)/10);
            input_snr = repmat(input_snr,1,50);
            SINR_p = 10*log10(input_snr.*abs_channel);
            
        case 2
            ch1_tmp = data_all(:,2:2:100);
            ch2_tmp = data_all(:,3:2:101);
            abs_channel1 = (abs(ch1_tmp).^2)/2;
            abs_channel2 = (abs(ch2_tmp).^2)/2;
            input_snr = 10.^(data_all(:,1)/10);
            input_snr = repmat(input_snr,1,50);
            SINR_p = 10*log10(input_snr.*abs_channel1 + input_snr.*abs_channel2);%- 10*log10(2);
            
        case 5
            prec1_tmp = data_all(:,203:end-1);
            prec2_tmp = arrayfun(@opposite_q,prec1_tmp);
            q1 = arrayfun(@demap_q,prec1_tmp);
            %q2 = q1;
            q2 = arrayfun(@demap_q,prec2_tmp);
            ch1_tmp = data_all(:,2:2:100);
            ch2_tmp = data_all(:,3:2:101);
            snr_tmp = 10.^(data_all(:,1)/10);
            snr_tmp = repmat(snr_tmp,1,50);
            
            alpha =(1/2)*abs(ch1_tmp + q1.*ch2_tmp);
            seta = (1/2)*((abs(ch1_tmp + q2.*ch2_tmp)));
            
            if (abs_mode==2 )
                SINR_p = 10*log10(snr_tmp.*(alpha.^2));%-10*log10(2);
            else
                SINR_p = 10*log10((snr_tmp.*(alpha.^2)) ./ ((snr_tmp.*(seta.^2)) + 1));
            end
            
        case 6
            prec_tmp = data_all(:,203:end-1);
            q = arrayfun(@demap_q,prec_tmp);
            ch1_tmp = data_all(:,2:2:100);
            ch2_tmp = data_all(:,3:2:101);
            snr_tmp = 10.^(data_all(:,1)/10);
            snr_tmp = repmat(snr_tmp,1,50);
            ch_composite = (abs(ch1_tmp + q.*ch2_tmp))/sqrt(2);
            SINR_p = 10*log10((snr_tmp.*((ch_composite).^2)));
            
    end
    
    %***************************************************************************************
    % for coarse beta calculation:
    %***************************************************************************************
    %     beta_vec = 1:0.1:30;
    %     delta = zeros(size(beta_vec));
    %     for beta=1:length(beta_vec)
    %         delta(beta)=delta_BLER_1((beta_vec(beta)));
    %     end
    %     [val ind] = min(delta);
    %     opt_beta_new(mcs(m)) = beta_vec(ind);
    %***************************************************************************************
    % for optmimized betas calculation using fminsearch
    %***************************************************************************************
    %beta_out=[1.0151 1.0565];
    beta_out=zeros(1,2);
    %[opt_beta(mcs(m)), MSE] = fminsearch(@delta_BLER_1,opt_beta_new(mcs(m)))
    [beta_out, std_deviat] = fminsearch(@polyfit_delta_BLER,[1 1])
    
    %***************************************************************************************
    % Calculation of Effective SINR based on optimized beta values
    %***************************************************************************************
    
    if(abs_mode==0) %MIESM Mapping
        
        eval(['load ' '/homes/latif/devel/trunk/openair1/SIMULATION/LTE_PHY/Abstraction/siso_MI_abs_' num2str(modu) 'Qam.mat'])
        
        [xize y]= size(SINR_p);
        RBIR = [];
        for t=1:1:xize
            s = SINR_p(t,:);
            s(s<-10)=-10;
            s(s>49)=49;
            eval(['SI_p = interp1(newSNR,newC_siso_' num2str(modu) 'QAM,s, ''linear'' , ''extrap'');']);
            RBIR(t) = (sum(SI_p/beta_out(1))/Pm);
            %RBIR(t) = (sum(SI_p)/Pm);
        end
        SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear');
        SINR_eff = SINR_eff .* beta_out(2);
        
        
    elseif(abs_mode==2)
            
            eval(['load ' '/homes/latif/devel/trunk/openair1/SIMULATION/LTE_PHY/Abstraction/siso_MI_abs_' num2str(modu) 'Qam.mat'])
            
            
            if(tx_mode~=5)
                error('IA-MIESM only for LTE TM5!!!')
            end
            
            if(modu==4)
                eval(['load ' '/homes/latif/devel/trunk/openair1/EMOS/LTE/POST_PROCESSING/Mode_5/mat_Files/data_mode5_4_' num2str(modu) '_morerealizations.mat'])
            else
                if(modu==16)
                    eval(['load ' '/homes/latif/devel/trunk/openair1/EMOS/LTE/POST_PROCESSING/Mode_5/mat_Files/data_mode5_16_16.mat'])
                end
            end
            
            [xize y]= size(SINR_p);
            RBIR = [];
            for t=1:1:xize
                s = SINR_p(t,:);
                s(s<-10)=-10;
                s(s>49)=49;
                
                a = round(alpha(t,:)*100);
                a(a==0) = 1;
                a(a>200)=200;
                b = round(seta(t,:)*100);
                b(b==0) = 1;
                b(b>200)=200;
                
                for u=1:length(a)
                    if(modu==16)
                        SI_p(u) = data_mode5_16_16(b(u) ,a(u), round(s(u) + 11));
                    else
                        SI_p(u) = data_mode5_4_4(b(u) ,a(u), round(s(u) + 11));
                    end
                end
                RBIR(t) = (sum(SI_p/beta_out(1))/Pm);
            end
            
            SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear');
            SINR_eff = SINR_eff .* beta_out(2);
            %SINR_eff = 10*log10((10.^(SINR_eff/10)) .* beta_out(2));
            
        else  %% EESM Mapping
            
            SINR_eff= 10*log10(-1*beta_out(2)*log((1/p)*sum(exp((-10.^(SINR_p'./10))./beta_out(1)))));
        end
        
        %***************************************************************************************
        % PolyNomial Calculation
        %***************************************************************************************
        
        [po s mu] = polyfit(SINR_eff,((BLER_meas))',3);
        [f] = polyval(po,SINR_eff,s,mu);
        
        %***************************************************************************************
        %MSE Calculation
        %***************************************************************************************
        %     MSE_final(mcs(m)+1) =  mean((SINR_awgn - SINR_eff).^2);
        Optimum_beta1(mcs(m)+1) = beta_out(1);
        Optimum_beta2(mcs(m)+1) = beta_out(2);
        %***************************************************************************************
        % BLER Plot w.r.t Mean Effective SINR
        %***************************************************************************************
        % figure;
        % avg_SINR_p = mean(SINR_p');
        % hist(avg_SINR_p)
        % s = strcat('Average_SINR_p, MCS = ',num2str(mcs(m)));
        % title(2)
        % figure;
        % grid on
        % semilogy(avg_SINR_p,BLER_meas,plot_style{m+1})
        % xlim([-8 15])
        % ylim([1e-3 1])
        % s = strcat('LTE TM1 Avergae Abstraction MCS ', num2str(mcs(m)));
        %     title(s);
        %     ylabel 'BLER'
        %     xlabel 'SINR_{average}'
        %     s2 = strcat('BLER_{meas} MCS ', num2str(mcs(m)));
        %     legend(s2, 'Location',  'Best');
        
        %***************************************************************************************
        % BLER Plot w.r.t Effective SINR
        %***************************************************************************************
        %     figure;
        %     hist(SINR_eff)
        %     s = strcat('SINR_eff, MCS = ',num2str(mcs(m)));
        %     title(s)
        h_fig = figure;
        semilogy(SINR_eff,BLER_meas,plot_style{m+1})
        hold on
        grid on
        semilogy(SINR_eff,f,'mx');
        xlim([-20 20])
        ylim([1e-3 1])
        s = strcat(ss, ',beta1= ', num2str(Optimum_beta1(mcs(m)+1)),', beta2= ',num2str(Optimum_beta2(mcs(m)+1)));
        title(s);
        ylabel 'BLER'
        xlabel 'SINR_{effective}'
        s2 = strcat('BLER_{meas} MCS ', num2str(mcs(m)));
        s3 = strcat('BLER_{AWGN} MCS ', num2str(mcs(m)));
        legend(s2,s3, 'Location',  'Best');
        
        if(save_out==1)
            saveas(h_fig,fullfile(pathname,strcat('IA_RX_M1-MIESM_TX5_mcs',num2str(mcs(m)) ,'.eps')),'epsc2');
            saveas(h_fig,fullfile(pathname,strcat('IA_RX_M1-MIESM_TX5_mcs',num2str(mcs(m)) ,'.fig')),'fig');
            saveas(h_fig,fullfile(pathname,strcat('IA_RX_M1-MIESM_TX5_mcs',num2str(mcs(m)) ,'.jpg')),'jpg');
            
            fprintf(fid,'\\begin{figure}[!h]\n');
            fprintf(fid,'\\centering\n');
            fprintf(fid,'\\includegraphics[scale=0.5]{%s}\n',fullfile(pathname,strcat('IA_RX_M1_MIESM_TX5_mcs', num2str(mcs(m)),'.eps')));
            fprintf(fid,'\\caption{%s}\n',s);
            fprintf(fid,'\\label{fig:mcs%d}\n',mcs(m));
            fprintf(fid,'\\end{figure}\n\n');
        end
    end
    if(save_out)
        fprintf(fid,'\\end{document}\n');
        fclose(fid);
    end
    
    %***************************************************************************************
    %For saving the files
    %***************************************************************************************
    % if(save_out)
    %     save(strcat('opt_beta_mcs',num2str(mcs(m)),'.dat'),'Optimum_beta*')
    %     save(strcat('MSE_mcs',num2str(mcs(m)),'.dat'),'MSE_final')
    % end
    
    
