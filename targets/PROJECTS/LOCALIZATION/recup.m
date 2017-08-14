clear all

addpath([getenv('OPENAIR_TARGETS') '/ARCH/EXMIMO/USERSPACE/OCTAVE'])

Nmeas=1
rx_spec
Niter=1
meas=1
for meas=1:Nmeas

close all

receivedA2B1=oarf_get_frame(card);
%oarf_stop(card);
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

%load where2_v1_final.mat data
load filesave0811.mat data
Nofs=3840; % The offset in samples between the downlink and uplink sequences. It must be a multiple of 640,  3840 samples is 0.5 ms.
maskA2B=kron(ones(1,N/(2*Nofs)),[ones(1,Nofs) zeros(1,Nofs)])';
datamaskA2B=diag(kron(ones(1,N/(2*Nofs)),[ones(1,Nofs/640) zeros(1,Nofs/640)]));
%signalA2B=s2;
Da2b_T=datamaskA2B*data;
%tmps=s2;

Da2b_R=zeros(Niter*120,Nantb*301);
  
 for i=0:119;
    ifblock=receivedA2B(i*640+[1:640],indB);
    ifblock(1:128,:)=[];
    fblock=fft(ifblock);
    fblock(1,:)=[];
    fblock(151:360,:)=[];
    Da2b_R((Niter-1)*120+i+1,:,meas)=vec(fblock);	      
  endfor
  HA2B=repmat(conj(Da2b_T),Niter,Nantb).*Da2b_R(:,:,meas);
  phasesA2B=unwrap(angle(HA2B));
  if(mean(var(phasesA2B))>0.5) 
    disp("The phases of your estimates from A to B are a bit high (larger than 0.5 rad.), something is wrong.");
  endif
  chanestsA2B(:,:,meas)=reshape(diag(repmat(Da2b_T,Niter,Nantb)'*Da2b_R(:,:,meas))/size(Da2b_T,1),301,Nantb);
  #fchanestsA2B=zeros(512,Nantb);
  #for i=1:Nantb
  #  fchanestsA2B(:,i)=[0; chanestsA2B([1:150],i,meas); zeros(210,1); chanestsA2B(151:301,i,meas)];
  #endfor
  fchanestsA2B(:,:,meas)=[zeros(1,Nantb); chanestsA2B([1:150],:,meas); zeros(210,Nantb); chanestsA2B(151:301,:,meas)];
  tchanestsA2B(:,:,meas)=ifft(fchanestsA2B(:,:,meas));

%keyboard
end %% Nmeas

  received = receivedA2B;
  phases = phasesA2B;
  tchanests = [tchanestsA2B(:,:,end)];% , tchanestsB2A(:,:,end)];
  fchanests = [fchanestsA2B(:,:,end)];% , fchanestsB2A(:,:,end)];

plots
