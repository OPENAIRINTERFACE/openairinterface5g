% do some beamforming
signalA2B=zeros(N,4);
signalB2A=zeros(N,4);
signalB2A_prec_full=zeros(N,4);
signalB2A_prec_diag=zeros(N,4);
Da2b_R=zeros(Niter*120,Nantb*301);
Db2a_R=zeros(Niter*120,Nanta*301);

%%----------Node A to B transmission---------%%
for i=1:4
    if(active_rfA(i))
        [Da2b_T, tmps]=genrandpskseq(N,M,amp);
        signalA2B(:,i)=tmps*2; %make sure LSB is 0 (switch=tx)
    else
        signalA2B(:,i)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
    end
end

oarf_send_frame(card,signalA2B,n_bit);
%keyboard
sleep(0.01);
receivedA2B=oarf_get_frame(card);
%oarf_stop(card); %not good, since it does a reset
sleep(0.01);

%% ------- Do the A to B channel estimation ------- %%
for i=0:119;
    ifblock=receivedA2B(i*640+[1:640],indB);
    ifblock(1:128,:)=[];
    fblock=fft(ifblock);
    fblock(1,:)=[];
    fblock(151:360,:)=[];
    Da2b_R(i+1,:)=vec(fblock);
end
chanestsA2B=reshape(diag(repmat(Da2b_T,1,Nantb)'*Da2b_R)/size(Da2b_T,1),301,Nantb);
%fchanestsA2B(:,:,meas)=[zeros(1,Nantb); chanestsA2B([1:150],:,meas); zeros(210,Nantb); chanestsA2B(151:301,:,meas)];
%tchanestsA2B(:,:,meas)=ifft(fchanestsA2B(:,:,meas));

%% calculate beamformer based on chanestA2B 
for i=1:301
    YA=chanestsA2B(i,:).';
    F=Fs(:,:,i);
    Fd=Fds(:,:,i);
    BFs(:,i) = conj(F*YA)./norm(F*YA);
    BDs(:,i) = conj(Fd*YA)./norm(Fd*YA);
end

%% generate normal and beamformed signals
[seqf, tmps]=genrandpskseq(N,M,amp);
for i=1:size(seqf,1)
    % precoding
    for j=1:size(seqf,2)
        seqf_prec_full(:,i,j)=BFs(:,j)*seqf(i,j);
        seqf_prec_diag(:,i,j)=BDs(:,j)*seqf(i,j);
    end
end

tmps_prec_diag = ofdm_mod(seqf_prec_full,amp);
tmps_prec_full = ofdm_mod(seqf_prec_diag,amp);

%%
for i=1:4
    if(active_rfB(i))
      signalB2A(:,i)=floor(tmps/sqrt(3))*2; %make sure LSB is 0 (switch=tx)
      signalB2A_prec_full(:,i)=tmps_prec_full(i-1,:)*2; %make sure LSB is 0 (switch=tx)
      signalB2A_prec_diag(:,i)=tmps_prec_diag(i-1,:)*2; %make sure LSB is 0 (switch=tx)
    else
      signalB2A(:,i)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
      signalB2A_prec_full(:,i)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
      signalB2A_prec_diag(:,i)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
    end
end

%% send normal signal
P_tx = 10*log10(sum(mean(abs(signalB2A(:,2:4)).^2)))
oarf_send_frame(card,signalB2A,n_bit);
%keyboard
sleep(0.01);
receivedB2A=oarf_get_frame(card);
sleep(0.01);
% measure SNR
P_rx = 10*log10(mean(abs(receivedB2A(:,1)).^2))

%% send beamformed DL signal
P_tx_prec_diag = 10*log10(sum(mean(abs(signalB2A_prec_diag(:,2:4)).^2)))
oarf_send_frame(card,signalB2A_prec_diag,n_bit);
%keyboard
sleep(0.01);
receivedB2A_prec_diag=oarf_get_frame(card);
sleep(0.01);
% measure SNR
P_rx_prec_diag = 10*log10(mean(abs(receivedB2A_prec_diag(:,1)).^2))

%% send beamformed DL signal
P_tx_prec_full = 10*log10(sum(mean(abs(signalB2A_prec_full(:,2:4)).^2)))
oarf_send_frame(card,signalB2A_prec_full,n_bit);
%keyboard
sleep(0.01);
receivedB2A_prec_full=oarf_get_frame(card);
sleep(0.01);
% measure SNR
P_rx_prec_full = 10*log10(mean(abs(receivedB2A_prec_full(:,1)).^2))

figure(10)
bar([P_tx P_tx_prec_diag P_tx_prec_full; P_rx P_rx_prec_diag P_rx_prec_full]);
legend('normal','beamformed F diag', 'beamformed F full')
ylim([0 60])
drawnow
