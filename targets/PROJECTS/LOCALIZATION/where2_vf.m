clear all
Nmeas=1
rx_spec

for meas=1:Nmeas

close all

receivedA2B1=oarf_get_frame(card);

initial_sync

N=76800;

indA=[1 0 0 0];
indB=[1 2 3 4];

Nanta=length(indA);
Nantb=length(indB);

fchan=zeros(512,Nantb);
tchan=fchan;

frame_start=abs(frame_start)

I= [frame_start:N,1:frame_start-1];

receivedA2B=receivedA2B1(I',:);

%load where2_v1_final.mat 
load filesave0811.mat
Nofs=3840; % The offset in samples between the downlink and uplink sequences. It must be a multiple of 640,  3840 samples is 0.5 ms.
maskA2B=kron(ones(1,N/(2*Nofs)),[ones(1,Nofs) zeros(1,Nofs)])';
datamaskA2B=diag(kron(ones(1,N/(2*Nofs)),[ones(1,Nofs/640) zeros(1,Nofs/640)]));
signalA2B=s2;
Da2b_T=datamaskA2B*data;
%tmps=s2;
Niter=1

Da2b_R=zeros(Niter*120,Nantb*301);
  
for i=0:119;
    ifblock=receivedA2B(i*640+[1:640],indB);
    ifblock(1:128,:)=[];
    fblock=fft(ifblock);
    fblock(1,:)=[];
    fblock(151:360,:)=[];
    Da2b_R((Niter-1)*120+i+1,:)=vec(fblock);	      
endfor

HA2B=repmat(conj(Da2b_T),1,Nantb).*Da2b_R;
phasesA2B=unwrap(angle(HA2B));
  
chanestsA2B=reshape(diag(repmat(Da2b_T,1,Nantb)'*Da2b_R)/size(Da2b_T,1),301,Nantb);
fchanestsA2B=zeros(512,Nantb);
for i=1:Nantb
  fchanestsA2B(:,i)=[0; chanestsA2B([1:150],i); zeros(210,1); chanestsA2B(151:301,i)];
endfor

tchanestsA2B=ifft(fchanestsA2B);

fchan=fchanestsA2B+fchan;

end %% Nmeas

siglevel0_dB=10*log10(sum(abs((receivedA2B(:,1)-mean(receivedA2B(:,1))).^2))/length(receivedA2B(:,1)));
siglevel1_dB=10*log10(sum(abs((receivedA2B(:,2)-mean(receivedA2B(:,2))).^2))/length(receivedA2B(:,2)));
siglevel2_dB=10*log10(sum(abs((receivedA2B(:,3)-mean(receivedA2B(:,3))).^2))/length(receivedA2B(:,3)));
siglevel3_dB=10*log10(sum(abs((receivedA2B(:,4)-mean(receivedA2B(:,4))).^2))/length(receivedA2B(:,4)));

fprintf("SigLevels : %2.2f dB,%2.2f dB,%2.2f dB,%2.2f dB\n",siglevel0_dB,siglevel1_dB,siglevel2_dB,siglevel3_dB);

received = receivedA2B;

fchanests = [fchan(:,:)/Nmeas];

tchanests = ifft((fchan));

plots
