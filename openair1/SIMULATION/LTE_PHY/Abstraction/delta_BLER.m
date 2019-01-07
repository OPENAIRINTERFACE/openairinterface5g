function delta=delta_BLER(beta)

beta1 =beta(1);
beta2 =beta(2);
global SINR_p BLER_meas snr bler SINR_awgn abs_mode modu Pm tx_mode alpha seta

p = 50;
if(abs_mode==1)
SINR_eff= 10*log10(-1*beta1*log((1/p)*sum(exp((-10.^(SINR_p'./10))./beta2))));
else
    
    eval(['load ' '/homes/latif/devel/trunk/openair1/SIMULATION/LTE_PHY/siso_MI_abs_' num2str(modu) 'Qam.mat'])
    if(tx_mode==5)
       eval(['load ' '/homes/latif/devel/trunk/openair1/EMOS/LTE/POST_PROCESSING/Mode_5/mat_Files/data_mode5_4_' num2str(modu) '_new.mat']) 
    end
    [x y]= size(SINR_p);
    RBIR = [];
    for t=1:1:x
        s = SINR_p(t,:);
%         s = (10.^(s/10))./beta1;
%         s = 10*log10(s);
        if(tx_mode~=5)
        eval(['SI_p = interp1(newSNR,newC_siso_' num2str(modu) 'QAM,s, ''linear'' , ''extrap'');']);
        
        else
             s(s<-10) = -10;
            a = round(alpha(t,:)*100);
            a(a==0) = 1; 
            a(a>200)=200; 
            b = round(seta(t,:)*100);
            b(b==0) = 1; 
            b(b>200)=200;
            
            for u=1:length(a)
            SI_p(u) = data_mode5_4_4_2(b(u) ,a(u), round(s(u) + 11));
            end
        end
        %I(t) = mean(SI_p);
        RBIR(t) = sum(SI_p/beta1)/Pm;
    end
    SINR_eff = interp1(newRBIR, newSNR, RBIR,'linear'); 
    %SINR_eff = interp1(newC_siso_16QAM,newSNR,I, 'linear' , 'extrap');
    
    %SINR_eff = 10*log10(beta1.*(10.^SINR_eff/10));
end

BLER_pred = interp1(snr,bler,SINR_eff,'linear','extrap');
BLER_pred(SINR_eff<snr(1)) = 1;
BLER_pred(SINR_eff>snr(end)) = 0;
%if any(isnan(BLER_pred))
%    error('shold never happen')
%end

BLER_pred =BLER_pred';

delta=sum((BLER_pred - BLER_meas).^2);

% SINR_eff= 10*log10(-1*beta1*log((1/p)*sum(exp((-10.^(SINR_p'./10))./beta2))));
% 
% delta = mean((SINR_awgn - SINR_eff).^2);