set(0, 'DefaultLineMarkerSize', 10);
set(0, 'Defaultaxesfontsize', 14);
set(0, 'DefaultLineLineWidth', 2);

root_path = 'AWGN/AWGN_results';
tx_mode = 1;
nrx = 1;
channel = 18;
nb_rb = 25; %RB used
nb_sf = 4;  %subframes per frame

%%
addpath('../../../EMOS/LTE/POST_PROCESSING')
load SISO.mat

%extended CP, 1PDCCH symbol
%scale_bps = 25*(8*12+3*10)*6*100; 
%normal CP, 1PDCCH symbol
scale_bps = nb_rb*(10*12+3*10)*nb_sf*100;
c_siso = [c_siso_4Qam; c_siso_16Qam; c_siso_64Qam];
bps_siso = c_siso.*scale_bps;

snr_all = -20:0.2:40;
throughput_all = zeros(length(snr_all),22);

plot_style = {'b-*';'r-*';'g-*';'y-*';'k-*';...
    'b-o';'r-o';'g-o';'y-o';'k-o';...
    'b-s';'r-s';'g-s';'y-s';'k-s';...
    'b-d';'r-d';'g-d';'y-d';'k-d';...
    'b-x';'r-x';'g-x';'y-x';'k-x';...
    'b-+';'r-+';'g-+';'y-+';'k-+'};


%%
figure(1)
hold off
figure(2)
hold off
legend_str = {};
i=1;
for mcs=0:28
    file = fullfile(root_path,sprintf('bler_tx%d_chan%d_nrx%d_mcs%d.csv',tx_mode,channel,nrx,mcs));
    %file = fullfile(root_path,sprintf('awgn_bler_tx%d_mcs%d.csv',tx_mode,mcs));
    try
        data = dlmread(file,';',1,0);
    catch exception
        disp(sprintf('Problem with file %s:',file));
        disp(exception.message);
        continue
    end
    snr = data(:,1);
    bler = data(:,5)./data(:,6); % round 1
    bler4 = data(:,11)./data(:,6); % round 4
    harq_adjust = data(:,6)./sum(data(:,6:2:12),2);
    if size(data,2)>13
        uncoded_ber = data(:,14);
    else
        uncoded_ber = ones(size(data,1),1);
    end
    if (length(snr)==1)
    throughput_all(:,mcs+1) = (1-bler4).*harq_adjust.*get_tbs(mcs,nb_rb)*nb_sf*100;
    else
    throughput_all(:,mcs+1) = interp1(snr, (1-bler4).*harq_adjust.*get_tbs(mcs,nb_rb)*nb_sf*100,snr_all,'nearest','extrap');
    end
    throughput_all(1:find(snr_all==snr(1)),mcs+1) = 0;
    figure(1)
    semilogy(snr,bler(:,1),plot_style{mcs+1});
    %snr10p(mcs+1) = interp1(bler(:,1),snr,0.1);
    hold on
    figure(2)
    semilogy(snr,uncoded_ber,plot_style{mcs+1});
    hold on
    legend_str{i} = sprintf('mcs %d',mcs);
    i=i+1;
end

%%
h_fig = figure(1);
h_leg = legend(legend_str,'location','eastoutside');
set(h_leg,'FontSize',10);
title(sprintf('Tx mode %d, channel %d (%s): BLER vs SNR',tx_mode, channel,root_path));
ylabel 'BLER'
xlabel 'SNR'
ylim([0.001 1])
xlim([-10 40])
grid on
saveas(h_fig,fullfile(root_path,sprintf('tx_mode%d_channel%d_bler.fig',tx_mode,channel)));
saveas(h_fig,fullfile(root_path,sprintf('tx_mode%d_channel%d_bler.emf',tx_mode,channel)));

h_fig = figure(2);
h_leg = legend(legend_str,'location','eastoutside');
set(h_leg,'FontSize',10);
title(sprintf('Tx mode %d: uncoded BER vs SNR',tx_mode));
ylabel 'uncoded BER'
xlabel 'SNR'
ylim([0.0001 1])
grid on
saveas(h_fig,fullfile(root_path,sprintf('tx_mode%d_channel%d_ber.fig',tx_mode,channel)));
saveas(h_fig,fullfile(root_path,sprintf('tx_mode%d_channel%d_ber.emf',tx_mode,channel)));

%%
h_fig = figure(4);
hold off
plot(snr_all,(max(throughput_all,[],2)))
%plot(snr_all,throughput_all)
hold on
plot(SNR,max(bps_siso),'r--');
legend('simulation','SISO mutual information','location','northwest');
xlabel('SNR [dB]')
ylabel('Throughput [bps]')
grid on

