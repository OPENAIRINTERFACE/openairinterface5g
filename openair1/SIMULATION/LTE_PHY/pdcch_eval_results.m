set(0, 'DefaultLineMarkerSize', 10);
set(0, 'Defaultaxesfontsize', 14);
set(0, 'DefaultLineLineWidth', 2);
plot_style = {'b-o';'r-x';'g-d';'y-*';'k-s'};

SNR = -5:0.2:2.8;
C = zeros(length(SNR),4);
filebase = 'pdcch_fdd_5_siso_awgn_format0';

figure(1)
hold off
for L=0:3
    printf("Opening %s\n",sprintf('%s_L%d.txt',filebase,L));
    fid = fopen(sprintf('%s_L%d.txt',filebase,L),'r');
    printf("fid %d\n",fid);
    [A,c]=fscanf(fid,'SNR %f : n_errors_ul = %d/%d (%f)\n');
    fclose(fid);

    B = reshape(A,4,40).';
    C(:,L+1) = B(:,4); 
    
    semilogy(B(:,1),B(:,4),plot_style{L+1})
    hold on
    
    legend_str{L+1} = sprintf('L=%d',L);
end

%%
h = legend(legend_str);
set(h,'Fontsize',14);
title(filebase,'Fontsize',16,'Interpreter','none')
xlabel('SNR','Fontsize',14)
ylabel('BLER','Fontsize',14)
grid on
saveas(gcf, filebase, 'eps');
