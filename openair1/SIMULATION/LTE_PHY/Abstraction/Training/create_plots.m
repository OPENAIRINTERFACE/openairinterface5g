  %% For dual Beta
  
    if(show_plots)
        h_fig = figure;
    else
    h_fig = figure('color',[1 1 1],'position',[178 1 934 708],'Visible','off');
    end
    
    semilogy(SINR_eff_dual_beta,BLER_meas,plot_style{m+1})
    hold on
    grid on
    semilogy(snr,bler,'m-x');
    xlim([-15 20])
    ylim([1e-3 1])
    if (tx_mode == 5)
        if (abs_mode==0)
        s = strcat('LTE TM 5 M2-MIESM MCS ', num2str(mcs(m)), ',beta1= ',num2str(dual_Optimum_Beta1(m)),',beta2= ',num2str(dual_Optimum_Beta2(m)), ', MSE= ',num2str(MSE_final_dual_beta(m)));
        ss = 'training_tm5_m2miesm_dual_beta';
        pathname = M2_path;
        else
            if (abs_mode ==2)
                s = strcat('LTE TM 5 M1-MIESM MCS ', num2str(mcs(m)), ',beta1= ',num2str(dual_Optimum_Beta1(m)),',beta2= ',num2str(dual_Optimum_Beta2(m)), ', MSE= ',num2str(MSE_final_dual_beta(m)));
                ss = 'training_tm5_m1miesm_dual_beta_noMF';
                pathname = M1_path;
            else
                s = strcat('LTE TM 5 EESM MCS ', num2str(mcs(m)), ',beta1= ',num2str(dual_Optimum_Beta1(m)),',beta2= ',num2str(dual_Optimum_Beta2(m)), ', MSE= ',num2str(MSE_final_dual_beta(m)));
                ss = 'training_tm5_eesm_dual_beta';
                pathname = EESM_path;
            end
        end
    else
        if (abs_mode==0)
        s = strcat('LTE TM ', num2str(tx_mode), ' MIESM MCS ', num2str(mcs(m)), ',beta1= ',num2str(dual_Optimum_Beta1(m)),',beta2= ',num2str(dual_Optimum_Beta2(m)), ', MSE= ',num2str(MSE_final_dual_beta(m)));
        ss = strcat('training_tm',num2str(tx_mode), '_miesm_dual_beta');
        pathname = MIESM_path;
        else
            s = strcat('LTE TM ', num2str(tx_mode), ' EESM MCS ', num2str(mcs(m)), ',beta1= ',num2str(dual_Optimum_Beta1(m)),',beta2= ',num2str(dual_Optimum_Beta2(m)), ', MSE= ',num2str(MSE_final_dual_beta(m)));
            ss = strcat('training_tm',num2str(tx_mode), '_eesm_dual_beta');
            pathname = EESM_path;
        end
    end
    
    title(s);
    ylabel 'BLER'
    xlabel 'SINR_{effective}'
    s2 = strcat('BLER_{meas} MCS ', num2str(mcs(m)));
    s3 = strcat('BLER_{AWGN} MCS ', num2str(mcs(m)));
    legend(s2,s3, 'Location',  'SouthWest');
    
    if(save_out)
        print(fullfile(pathname,strcat(ss, '_mcs', num2str(mcs(m)),'.eps')),'-dDEVICE','epsc2');
        print(fullfile(pathname,strcat(ss, '_mcs', num2str(mcs(m)),'.fig')),'-dDEVICE','fig');
        print(fullfile(pathname,strcat(ss, '_mcs', num2str(mcs(m)),'.jpg')),'-dDEVICE','jpg');
      
        fprintf(fid,'\\begin{figure}[htbp]\n');
        fprintf(fid,'\\centering\n');
        fprintf(fid,'\\includegraphics[scale=0.4]{%s}\n',fullfile(pathname,strcat(ss, '_mcs', num2str(mcs(m)),'.eps')));
        fprintf(fid,'\\caption{\\scriptsize{%s}}\n',s);
        fprintf(fid,'\\label{fig:mcs%d}\n',mcs(m));
        fprintf(fid,'\\end{figure}\n\n');
        
    end
   %% For Single Beta 
    
    if(show_plots)
        h_fig = figure;
    else
    h_fig = figure('color',[1 1 1],'position',[178 1 934 708],'Visible','off');
    end
    
    semilogy(SINR_eff_single_beta,BLER_meas,plot_style{m+1})
    hold on
    grid on
    semilogy(snr,bler,'m-x');
    xlim([-15 20])
    ylim([1e-3 1])
    if (tx_mode == 5)
        if (abs_mode==0)
        s = strcat('LTE TM 5 M2-MIESM MCS ', num2str(mcs(m)), ',beta1= beta2= ',num2str(single_Optimum_Beta(m)),', MSE= ',num2str(MSE_final_single_beta(m)));
        ss = 'training_tm5_m2miesm_single_beta';
        pathname = M2_path;
        else
            if (abs_mode ==2)
                s = strcat('LTE TM 5 M1-MIESM MCS ', num2str(mcs(m)), ',beta1= beta2= ',num2str(single_Optimum_Beta(m)),', MSE= ',num2str(MSE_final_single_beta(m)));
                ss = 'training_tm5_m1miesm_single_beta_noMF';
                pathname = M1_path;
            else
                s = strcat('LTE TM 5 EESM MCS ', num2str(mcs(m)), ',beta1= beta2= ',num2str(single_Optimum_Beta(m)),', MSE= ',num2str(MSE_final_single_beta(m)));
                ss = 'training_tm5_eesm_single_beta';
                pathname = EESM_path;
            end
        end
    else
        if (abs_mode==0)
        s = strcat('LTE TM ', num2str(tx_mode), ' MIESM MCS ', num2str(mcs(m)), ',beta1= beta2= ',num2str(single_Optimum_Beta(m)),', MSE= ',num2str(MSE_final_single_beta(m))); 
        ss = strcat('training_tm',num2str(tx_mode), '_miesm_single_beta');
        pathname = MIESM_path;
        else
            s = strcat('LTE TM ', num2str(tx_mode), ' EESM MCS ', num2str(mcs(m)), ',beta1= beta2= ',num2str(single_Optimum_Beta(m)),', MSE= ',num2str(MSE_final_single_beta(m)));
            ss = strcat('training_tm',num2str(tx_mode), '_eesm_single_beta');
            pathname = EESM_path;
        end
    end
    
    title(s);
    ylabel 'BLER'
    xlabel 'SINR_{effective}'
    s2 = strcat('BLER_{meas} MCS ', num2str(mcs(m)));
    s3 = strcat('BLER_{AWGN} MCS ', num2str(mcs(m)));
    legend(s2,s3, 'Location',  'SouthWest');
    
    if(save_out)
        print(fullfile(pathname,strcat(ss, '_mcs', num2str(mcs(m)),'.eps')),'-dDEVICE','epsc2');
        print(fullfile(pathname,strcat(ss, '_mcs', num2str(mcs(m)),'.fig')),'-dDEVICE','fig');
        print(fullfile(pathname,strcat(ss, '_mcs', num2str(mcs(m)),'.jpg')),'-dDEVICE','jpg');
        
        fprintf(fid,'\\begin{figure}[htbp]\n');
        fprintf(fid,'\\centering\n');
        fprintf(fid,'\\includegraphics[scale=0.4]{%s}\n',fullfile(pathname,strcat(ss, '_mcs', num2str(mcs(m)),'.eps')));
        fprintf(fid,'\\caption{\\scriptsize{%s}}\n',s);
        fprintf(fid,'\\label{fig:mcs%d}\n',mcs(m));
        fprintf(fid,'\\end{figure}\n\n');
        if (rem(m,2)==0)
        fprintf(fid,'\\clearpage\n\n');
        end
        
    end
    
