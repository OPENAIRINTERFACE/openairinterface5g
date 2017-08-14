function [SINR_eff]=sinr_Eff_Calc(beta)
global SINR_p abs_mode modu Pm tx_mode alpha seta p mi_file mi_2_file


if(length(beta)==2)
    
    if(abs_mode==0) %MIESM Mapping
        
        
        eval('load(mi_file)')
        
        [xize y]= size(SINR_p);
        RBIR = [];
        for t=1:1:xize
            s = SINR_p(t,:);
            s(s<-10)=-10;
            s(s>49)=49;
            eval(['SI_p = interp1(newSNR,newC_siso_' num2str(modu) 'QAM,s, ''linear'' , ''extrap'');']);
            RBIR(t) = (sum(SI_p/beta(1))/Pm);
            %RBIR(t) = (sum(SI_p)/Pm);
        end
        SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear');
        SINR_eff = SINR_eff * beta(2);
        
        
    elseif(abs_mode==2)
        
       eval('load(mi_file)')
        
        if(tx_mode~=5)
            error('IA-MIESM only for LTE TM5!!!')
        end
        
        eval('load(mi_2_file)')
        
        [xize y]= size(SINR_p);
        RBIR = [];
        for t=1:1:xize
            s = SINR_p(t,:);
            s(s<-10)=-10;
            s(s>37)=38;
            
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
            RBIR(t) = (sum(SI_p/beta(1))/Pm);
        end
        
        SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear','extrap');
        SINR_eff = SINR_eff .* beta(2);
        %SINR_eff = 10*log10((10.^(SINR_eff/10)) * beta_out(2));
        
    else  %% EESM Mapping
        
        SINR_eff= 10*log10(-1*beta(2)*log((1/p)*sum(exp((-10.^(SINR_p'./10))./beta(1)))));
    end
    
else
    
    if(abs_mode==0) %MIESM Mapping
        
        
        eval('load(mi_file)')
        
        [xize y]= size(SINR_p);
        RBIR = [];
        for t=1:1:xize
            s = SINR_p(t,:);
            s(s<-10)=-10;
            s(s>49)=49;
            eval(['SI_p = interp1(newSNR,newC_siso_' num2str(modu) 'QAM,s, ''linear'' , ''extrap'');']);
            RBIR(t) = (sum(SI_p/beta)/Pm);
            %RBIR(t) = (sum(SI_p)/Pm);
        end
        SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear');
        SINR_eff = SINR_eff * beta;
        
        
    elseif(abs_mode==2)
        
       eval('load(mi_file)')
        
        if(tx_mode~=5)
            error('IA-MIESM only for LTE TM5!!!')
        end
        
        eval('load(mi_2_file)')
        
        [xize y]= size(SINR_p);
        RBIR = [];
        for t=1:1:xize
            s = SINR_p(t,:);
            s(s<-10)=-10;
            s(s>37)=38;
            
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
            RBIR(t) = (sum(SI_p/beta)/Pm);
        end
        
        SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear','extrap');
        SINR_eff = SINR_eff .* beta;
        %SINR_eff = 10*log10((10.^(SINR_eff/10)) * beta_out(2));
        
    else  %% EESM Mapping
        
        SINR_eff= 10*log10(-1*beta*log((1/p)*sum(exp((-10.^(SINR_p'./10))./beta))));
    end
end
