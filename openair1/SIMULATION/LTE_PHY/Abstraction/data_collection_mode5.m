close all;
clear all;
warning on;
data_mode5 = zeros(200,200,50);
cnt = 1;
for f = 1:5
    eval(['load ''QPSK_QPSK_noMF_lte_Mode5_''' num2str(f) '.mat']);
    
    for i = 1:10      
        [gx, gy, gz] = gridfit(abs(alpha_1(i,:)).',abs(beta_2(i,:)).',chcap_lte_sch1(i,:).',0.01:0.01:2, 0.01:0.01:2 ); 
        gx(gx<0) = 0;
        gx(gx>2) = 2;
        data_mode5(:,:,cnt) = gx;
        cnt = cnt+1;
    end
end

save('data_mode5.mat', 'data_mode5');
