%clear all
%close all

addpath([getenv('OPENAIR1_DIR') '/PHY/LTE_REFSIG/']);
addpath([getenv('OPENAIR1_DIR') '/PHY/LTE_TRANSPORT/']);
primary_synch; %loads the primary sync signal
%sss_gen; %loads the secondary sync signal

% grab a frame


% load frame
%addpath([getenv('OPENAIR_TARGETS') '/SIMU/USER/']);
%eNBtxsig1



%keyboard

%figure(1)
%plot(real(s))

%pss_t = upsample(primary_synch0_time,4);
   primary_synch;
   primary_synch0_time;
   pss0_up = interp(primary_synch0_time,4);
   pss0_up_cp = [pss0_up(512-127:end) pss0_up];
  % plot(real(pss0_up_cp))
   pss0_up_cp = [pss0_up(512-127:end) pss0_up]*1024;
  % plot(real(pss0_up_cp))
   pss_t = pss0_up_cp;


s = receivedA2B1;

[corr,lag] = xcorr(s(:,1).',pss_t.');

[pss_amp, pss_pos0] = max(abs(corr));
pss_pos = lag(pss_pos0);
pss_ref=pss_pos;

frame_start=pss_ref

