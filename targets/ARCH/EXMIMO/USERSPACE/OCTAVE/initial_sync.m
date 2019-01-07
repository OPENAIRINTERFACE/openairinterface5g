clear all
close all

addpath([getenv('OPENAIR1_DIR') '/PHY/LTE_REFSIG/']);
addpath([getenv('OPENAIR1_DIR') '/PHY/LTE_TRANSPORT/']);
primary_synch; %loads the primary sync signal
sss_gen; %loads the secondary sync signal

% grab a frame
ch=1;
rx_spec;

% load frame
%addpath([getenv('OPENAIR_TARGETS') '/SIMU/USER/']);
%eNBtxsig1
%s = txs1;

figure(1)
plot(real(s))

pss_t = upsample(primary_synch0_time,4);
[corr,lag] = xcorr(s(:,ch).',pss_t);
figure(2);
plot(lag,abs(corr));

[pss_amp, pss_pos0] = max(abs(corr));
pss_pos = lag(pss_pos0);
%pss_pos = 8817;

%sss_pos = pss_pos - 548; %FDD normal CP
%sss_pos = pss_pos - 640; %FDD extended CP
sss_pos = pss_pos - 1648; %TDD normal CP
%sss_pos = pss_pos - 1920; %TDD extended CP

psst = s([pss_pos:(pss_pos+511)],ch);
pssf = fft(psst);
ssst = s([sss_pos:(sss_pos+511)],ch);
sssf = fft(ssst);

pssf_ext = pssf([(512-30):512 2:32]); % extract innter 62 subcarriers
sssf_ext = sssf([(512-30):512 2:32]); % extract innter 62 subcarriers

%estimate channel from pss
pss_ch = pssf_ext.*(primary_synch0(6:67)');
pss_ch = pss_ch./mean(abs(pss_ch));

%compensate sssf_ext with channel
sssf_comp = sssf_ext.*conj(pss_ch);

% frequency domain correlation (dot-wise product) of sssf_comp and d0 and d5
% account for phase offset

Nid2=0;
for Nid1=0:167
for phase=0; %-4:4
  corr0(Nid1+1,phase+1) = d0(Nid2+Nid1*3+1,:)*conj(sssf_comp*exp(1j*pi*phase/8));
  corr5(Nid1+1,phase+1) = d5(Nid2+Nid1*3+1,:)*conj(sssf_comp*exp(1j*pi*phase/8));  
end
end

figure(3)
plot(abs(corr0))
figure(4)
plot(abs(corr5))

[m0,i0] = max(abs(corr0(:)));
[m5,i5] = max(abs(corr5(:)));

if (m0>m5)
  frame_start = pss_pos - 8816
  phase = floor((i0-1)/168) %phase
  Nid1 = mod(i0-1,168) %Nid
else
  frame_start = pss_pos - 47216
  phase = floor((i5-1)/168) %phase
  Nid1 = mod(i5-1,168) %Nid
end

