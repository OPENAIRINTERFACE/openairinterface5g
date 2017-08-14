function plot_results(tm)
data_abs = dlmread(sprintf('UE_stats_abs_th0_tx%d.txt',tm));
data = dlmread(sprintf('UE_stats_th0_tx%d.txt',tm));
figure
plot(data(:,1),data(:,2))
hold on
plot(data_abs(:,1),data_abs(:,2),'r')
title(sprintf('Average Throughput of System for TM %d',tm))
xlabel('Frames')
ylabel('Throughput [kbps]')
legend('Full PHY','PHY ABSTRACTION')
hold off
%figure
%[f x]=ecdf(data(:,2));
%plot(x,f);
%hold on
%[f x]=ecdf(data_abs(:,2));
%plot(x,f);
%hold off
% figure
% cdfplot(data(:,2));
% hold on
% cdfplot(data_abs(:,2));
% hold off
%data0 = dlmread(sprintf('UE_stats_th0_tx%d.txt',tm));
%if(users>1)
%data1 = dlmread(sprintf('UE_stats_th1_tx%d.txt',tm));
%data1_abs = dlmread(sprintf('UE_stats_abs_th1_tx%d.txt',tm));
%end
%data0_abs = dlmread(sprintf('UE_stats_abs_th0_tx%d.txt',tm));
%figure
%plot(data0(:,1),data0(:,2))
%hold on; 
%plot(data0_abs(:,1),data0_abs(:,2),'r')
%title(sprintf('Instanteneous Throughput of UE 1 for TM %d',tm))
%xlabel('Frames')
%ylabel('Throughput [kbps]')
%legend('Full PHY','PHY ABSTRACTION')
%hold off
% figure
% [f x]=ecdf(data0(:,2));
% plot(x,f);
% hold on
% [f x]=ecdf(data0_abs(:,2));
% plot(x,f);
% hold off
%if(users>1)
%figure
%plot(data1(:,1),data1(:,2))
%hold on; 
%plot(data1_abs(:,1),data1_abs(:,2),'r')
%title(sprintf('Instanteneous Throughput of UE 2 for TM %d',tm))
%xlabel('Frames')
%ylabel('Throughput [kbps]')
%legend('Full PHY','PHY ABSTRACTION')
%hold off
%end
