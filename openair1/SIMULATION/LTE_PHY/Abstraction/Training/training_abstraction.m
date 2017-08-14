%***************************************************************************************
% Some basic initializations
%***************************************************************************************
global SINR_p BLER_meas snr bler SINR_awgn abs_mode modu Pm tx_mode alpha seta mi_file mi_2_file p

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

%***************************************************************************************
% Input Parameters used for the training of adjustment Factors (betas)
%***************************************************************************************
s2=[]; s3=[]; ss=[];

tx_mode=transmission_mode;
%1--> SISO, 2 --> Alamouti, 5 --> MU-MIMO ,6 --> TX Beamforming
% tells which transmission mode is to be abstracted.

abs_mode=abstraction_mode; 
% 0:MIESM (M2-MIESM), 1:EESM, 2:IA-MIESM (M1-MIESM)
%tells which abstraction methodology is to be used.

Pm=100; 
%2*50 QPSK
% Used for calculating RBIR

modu=4;
mi_file = strcat(userdir,'/SIMULATION/LTE_PHY/Abstraction/matFiles/siso_MI_abs_4Qam.mat');
mi_2_file = strcat(userdir,'/SIMULATION/LTE_PHY/Abstraction/matFiles/data_mode5_4_4_morerealizations.mat');
% Modulation used.

p=50;
save_out=save_output;

out_s1 = strcat(' Training for Transmission Mode = ', num2str(tx_mode));
out_s2 = strcat(' Training for Abstraction Mode = ', num2str(abs_mode));
out_s3 = strcat(' Saving Results = ', num2str(save_out));

disp('*******************************************************************');
disp(out_s1);
disp(out_s2);
disp(out_s3);
disp('*******************************************************************');

if (save_out)
    
    if(tx_mode==5)
        if(abs_mode==0)
            out_fname= fullfile(M2_path, strcat('tx_', num2str(tx_mode), '_abs_M2_MIESM'));
            out_s = 'M2 MIESM';
        else
            if (abs_mode==2)
                out_fname= fullfile(M1_path, strcat('tx_', num2str(tx_mode), '_abs_M1_MIESM'));
                out_s = 'M1 MIESM';
            else
                out_fname= fullfile(EESM_path, strcat('tx_', num2str(tx_mode), '_abs_EESM'));
                out_s = 'EESM';
            end
        end
    else
        if(abs_mode == 1)
        out_fname= fullfile(EESM_path, strcat('tx_', num2str(tx_mode), '_abs_EESM'));
        out_s = 'EESM';
        else
            out_fname= fullfile(MIESM_path, strcat('tx_', num2str(tx_mode), '_abs_MIESM'));
            out_s = 'MIESM';
        end
    end
    
fid = fopen(strcat(out_fname, '.tex'),'w');
fprintf(fid,'\\documentclass[a4paper,conference]{IEEEtran}\n');
fprintf(fid,'\\usepackage{epsfig}\n');
fprintf(fid,'\\usepackage{rotating}\n');
fprintf(fid,'\\usepackage{amsmath}\n');
fprintf(fid,'\\usepackage{amsfonts}\n');
fprintf(fid,'\\usepackage{amssymb}\n');
fprintf(fid,'\\usepackage{graphics}\n');
fprintf(fid,'\\usepackage{mathtools}\n');
fprintf(fid,'\\IEEEoverridecommandlockouts\n');
fprintf(fid,'\\title{%s}\n',sprintf('Results of %s for LTE Transmission Mode %d',out_s, tx_mode));
fprintf(fid,'\\author{\\IEEEauthorblockN{Imran Latif}\n');
fprintf(fid,'\\IEEEauthorblockA{Eurecom\\\\\n');
fprintf(fid,'2229, Route des Cretes, B.P. 193\\\\\n');
fprintf(fid,'06904 Sophia Antipolis, France\\\\\n');
fprintf(fid,'Email: (first name.last name)@eurecom.fr}\n');
fprintf(fid,'}\n');
fprintf(fid,'\\begin{document}\n');
fprintf(fid,'\\maketitle\n');
fprintf(fid,'\\begin{abstract}\n');
fprintf(fid,'\\end{abstract}\n');
fprintf(fid,'\\section{Results}\n');

end



%***************************************************************************************
% Main Execution Loop starts from here:
%***************************************************************************************
for m=1:1:length(mcs)
    
    if(mcs(m) >9 && mcs(m) < 17)
        modu=16;
        Pm=200; %4*50 16QAM
        mi_file = strcat(userdir,'/SIMULATION/LTE_PHY/Abstraction/siso_MI_abs_16Qam.mat');
        mi_2_file = strcat(userdir,'/SIMULATION/LTE_PHY/Abstraction/matFiles/data_mode5_16_16.mat');
    else if(mcs(m) > 16 && mcs(m) < 28)
            modu = 64;
            Pm=300; %6 *50  64QAM
            mi_file = strcat(userdir,'/SIMULATION/LTE_PHY/Abstraction/siso_MI_abs_64Qam.mat');
        end
    end
%***************************************************************************************
%Reference AWGN Curves
%***************************************************************************************
    data = dlmread(fullfile(awgn_path, strcat('awgn_bler_tx1_mcs', num2str(mcs(m)), '.csv')),';',1,0);
    A = data(:,1);
    B = data(:,5)./data(:,6); % round 1
       
    [bler I J] = unique(B);
    bler = sort(bler,1,'descend');
    I = sort(I,1,'ascend');
    snr = A(I);
    
    
%***************************************************************************************
% Data Extraction on Sub carrier basis data file saved using dlsim and reference
% SINR Calculation based on Reference AWGN Curve
%***************************************************************************************
    eval(['data_all = data_all' num2str(mcs(m)) ';']);
     data_all = data_all(data_all(:,end)<bler(1),:);
     data_all = data_all(data_all(:,end)>=bler(end),:);
    
    
    
    BLER_meas = data_all(:,end);
    
    SINR_awgn = interp1(bler, snr, BLER_meas,'linear','extrap');
    SINR_awgn = SINR_awgn';
    
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
            q2 = arrayfun(@demap_q,prec2_tmp);
            ch1_tmp = data_all(:,2:2:100);
            ch2_tmp = data_all(:,3:2:101);
            snr_tmp = 10.^(data_all(:,1)/10);
            snr_tmp = repmat(snr_tmp,1,50);
%             h1 = [ch1_tmp ch2_tmp];
%             p1 = [1; q1];
%             p2 = [1; q2];
            
            alpha =(1/2)*abs(ch1_tmp + q1.*ch2_tmp);
            %seta = abs(((conj(h1*p1))*(h1*p2))/(abs(h1*p1)));
            
            %seta = (1/2)*abs(((conj(ch1_tmp + q1.*ch2_tmp)).*(ch1_tmp + q2.*ch2_tmp))./(abs(ch1_tmp + q1.*ch2_tmp)));
  
            
            %alpha =sqrt(1/2)*abs(ch1_tmp + q1.*ch2_tmp);
            seta = (1/2)*((abs(ch1_tmp + q2.*ch2_tmp)));
%             alpha =abs(ch1_tmp + q1.*ch2_tmp);
            
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
 opt_beta=0;
 beta_out=ones(1,2);
 [opt_beta, MSE1] = fminsearch(@delta_BLER_1,1);
 
 [beta_out, MSE2] = fminsearch(@delta_BLER_1,[1 1]);

%***************************************************************************************
% Calculation of Effective SINR based on optimized beta values
%***************************************************************************************
   
SINR_eff_single_beta = sinr_Eff_Calc(opt_beta);
SINR_eff_dual_beta = sinr_Eff_Calc(beta_out);
 
    %***************************************************************************************
    %MSE Calculation
    %***************************************************************************************
    MSE_final_single_beta(m) =  mean((SINR_awgn - SINR_eff_single_beta).^2);
    MSE_final_dual_beta(m) =  mean((SINR_awgn - SINR_eff_dual_beta).^2);
    single_Optimum_Beta(m) = opt_beta;
    dual_Optimum_Beta1(m) = beta_out(1);
    dual_Optimum_Beta2(m) = beta_out(2);
    %***************************************************************************************
    % BLER Plot w.r.t Mean Effective SINR
    %***************************************************************************************
%     figure;
%     avg_SINR_p = mean(SINR_p');
%     hist(avg_SINR_p)
%     s = strcat('Average_SINR_p, MCS = ',num2str(mcs(m)));
%     title(2)
%     figure;
%     grid on
%     semilogy(avg_SINR_p,BLER_meas,plot_style{m+1})
%     xlim([-8 15])
%     ylim([1e-3 1])
%     s = strcat('LTE TM1 Avergae Abstraction MCS ', num2str(mcs(m)));
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


create_plots;
 
 

end

disp('      MCS       dual_Beta1   dual_Beta2     MSE      beta2=beta1       MSE');
disp([mcs' dual_Optimum_Beta1' dual_Optimum_Beta2' MSE_final_dual_beta' single_Optimum_Beta' MSE_final_single_beta'])
%***************************************************************************************
%For saving the files
%***************************************************************************************
if(save_out)
    
    fprintf(fid,'\\begin{table}[t]\n');
    fprintf(fid,'\\centering\n');
    fprintf(fid,'\\caption{beta1, beta2 and Mean Squared Error (MSE) Values for abstraction}\n');
    fprintf(fid,'\\begin{tabular}{ | c | c | c | c | c | c |}\n');
    fprintf(fid,'\\hline\n');
    fprintf(fid,'\\textbf{MCS} & \\textbf{Beta1} & \\textbf{Beta2} & \\textbf{MSE} & \\textbf{Beta1=Beta2} & \\textbf{MSE} \\\\ \\hline');
    for in=1:length(mcs)
        fprintf(fid,'%d & %3.5f & %3.5f & %01.5f & %3.5f & %01.5f \\\\ \\hline\n',mcs(in), dual_Optimum_Beta1(in), dual_Optimum_Beta2(in), MSE_final_dual_beta(in), single_Optimum_Beta(in), MSE_final_single_beta(in));
    end
    fprintf(fid,'\\end{tabular}\n');
    fprintf(fid,'\\end{table}\n');
    
    dlmwrite(fullfile(pathname,strcat(ss, '_beta1.csv')), dual_Optimum_Beta1, 'delimiter', ';');
    dlmwrite(fullfile(pathname,strcat(ss, '_beta2.csv')), dual_Optimum_Beta2, 'delimiter', ';');
    dlmwrite(fullfile(pathname,strcat(ss, '.csv')), single_Optimum_Beta, 'delimiter', ';');
    dlmwrite(fullfile(pathname,strcat(ss, '_mse.csv')), MSE_final_dual_beta, 'delimiter', ';');
    dlmwrite(fullfile(pathname,strcat(ss, '_mse.csv')), MSE_final_single_beta, 'delimiter', ';');
    fprintf(fid,'\\end{document}\n');
    fclose(fid);
    
    %system(sprintf('latex %s.tex', out_fname));
    %system(sprintf('dvipdf %s.dvi', out_fname));
    
end


