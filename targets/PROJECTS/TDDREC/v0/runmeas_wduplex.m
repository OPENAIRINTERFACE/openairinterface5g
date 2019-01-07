# % Author: Mirsad Cirkic
# % Organisation: Eurecom (and Linkoping University)
# % E-mail: mirsad.cirkic@liu.se

if(paramsinitialized && LSBSWITCH_FLAG)
  disp(["\n\n------------\nThis code is, so far, only written for single runs. Multiple " ... 
	"runs will overwrite the previous measurement data, i.e., the " ...
	"data structures are not defined for multiple runs. You will need to " ...
	"add code in order to save the intermediate measurements and the " ...
	"corresponding timestamps.\n------------"])
  N=76800;
  M=4;
  signalA2B=zeros(N,4);
  signalB2A=zeros(N,4);
  indA=find(active_rfA==1);
  indB=find(active_rfB==1);
  Nanta=length(indA);
  Nantb=length(indB);
  Niter=1;
  Nofs=3840; % The offset in samples between the downlink and uplink sequences. It must be a multiple of 640,  3840 samples is 0.5 ms.
  if(mod(Nofs,640)!=0) error("blabla") endif

  if(Nanta!=1) error("Node A can only have one antenna active\n"); endif
  if(Niter!=1) error("We should only use one get_frame at each run.\n"); endif
  
# %% ------- Prepare the signals for both A2B and B2A ------- %
  maskA2B=kron(ones(1,N/(2*Nofs)),[ones(1,Nofs) zeros(1,Nofs)])';
  maskB2A=ones(N,1)-maskA2B;
  datamaskA2B=diag(kron(ones(1,N/(2*Nofs)),[ones(1,Nofs/640) zeros(1,Nofs/640)]));
  datamaskB2A=eye(N/640)-datamaskA2B;
  signalA2B=zeros(N,4);
  signalB2A=zeros(N,4);
  ia=1; ib=1;
  Db2a_T=[];
  for i=1:4
    if(indA(ia)==i)
      [tmpd, tmps]=generqpskseq(N,M,amp);
      Da2b_T=datamaskA2B*tmpd;
      signalA2B(:,i)=tmps*4.*maskA2B+maskB2A*sqrt(-1); %Added maskB2A to set the LSB correctly. The factor 4 there should make a 2 bit shift. Don't know if that is correct
      if(length(indA)> ia) ia=ia+1; endif
    endif
    if(indB(ib)==i)      
      % This part could be improved by creating fully orthogonal sequences
      [tmpd, tmps]=generqpskseq(N,M,amp);
      signalB2A(:,i)=tmps*4.*maskB2A+maskA2B*sqrt(-1); %Added maskA2B to set the LSB correctly. The factor 4 there should make a 2 bit shift. Don't know if that is correct.
      Db2a_T=[Db2a_T datamaskB2A*tmpd];
      if(length(indB)> ib) ib=ib+1; endif
    endif
  endfor   

  signal = signalA2B+signalB2A;

# %% ------- Node B and A duplex transmission/reception ------- %%	
  rf_mode_current = rf_mode + (DMAMODE_TX+TXEN+DMAMODE_RX+RXEN)*active_rf;
  oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rx_gain,tx_gain,eNB_flag,rf_mode_current,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal_mode);
  oarf_send_frame(card,signal,n_bit);
  received=oarf_get_frame(card);
  oarf_stop(card);
  receivedA2B=received.*repmat(maskA2B,1,4);
  receivedB2A=received.*repmat(maskB2A,1,4);

# %% ------- Do the A to B channel estimation ------- %%	
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
  if(mean(var(phasesA2B))>0.5) 
    disp("The phases of your estimates are a bit high (larger than 0.5 rad.), something is wrong.");
  endif
  
  chanestsA2B=reshape(diag(repmat(Da2b_T,1,Nantb)'*Da2b_R)/size(Da2b_T,1),301,Nantb);
  fchanestsA2B=zeros(512,Nantb);
  for i=1:Nantb
    fchanestsA2B(:,i)=[0; chanestsA2B([1:150],i); zeros(210,1); chanestsA2B(151:301,i)];
  endfor
  tchanestsA2B=ifft(fchanestsA2B);
  
%% ------- Do the B to A channel estimation ------- %%
  Db2a_R=zeros(Niter*120,Nanta*301);
  for i=0:119;
    ifblock=receivedB2A(i*640+[1:640],indA);
    ifblock(1:128,:)=[];
    fblock=fft(ifblock);
    fblock(1,:)=[];
    fblock(151:360,:)=[];
    Db2a_R((Niter-1)*120+i+1,:)=fblock.';
  endfor  
  HB2A=conj(repmat(Db2a_T,Niter,1)).*repmat(Db2a_R,1,Nantb);
  phasesB2A=unwrap(angle(HB2A));
  if(mean(var(phasesB2A))>0.5) 
    disp("The phases of your estimates are a bit high (larger than 0.5 rad.), something is wrong.");
  endif
  chanestsB2A=reshape(diag(repmat(Db2a_T,Niter,1)'*repmat(Db2a_R,1,Nantb)/(Niter*120)),301,Nantb);
# %% -- Some plotting code -- %%  (you can uncomment what you see fit)
	# clf
	# figure(1)
	# for i=1:4 
	#   subplot(220+i);plot(20*log10(abs(fftshift(fft(receivedA2B(:,i)))))); 
	# endfor
	
	# figure(2)
	# t=[0:512-1]/512*1e-2;
	# plot(t,abs(tchanests))
	# xlabel('time')
	# ylabel('|h|')
	
	# figure(3)
	# % wndw = 50;
	# % for i=1:5:Nantb*301             %# sliding window size
	# %   phamean = filter(ones(wndw,1)/wndw, 1, phases(:,i)); %# moving average
	# %   plot(phamean(wndw:end),'LineWidth',2);
	# %   title(['subcarrier ' num2str(i)]);	  
	# %   xlabel('time')
	# %   ylabel('phase')
	# %   ylim([-pi pi])
	# %   drawnow;
	# %   pause(0.1)
	# % endfor
	# phavar=var(phases);
	# plotphavar=[];
	# for i=0:Nantb-1
	#   plotphavar=[plotphavar; phavar([1:301]+i*301)];
	# endfor
	# plot([1:150 362:512],plotphavar,'o');
	# %ylim([0 pi])
	# xlabel('subcarrier')
	# ylabel('phase variance')
	

	# figure(4)
	# plot(20*log10(abs(fchanests))), ylim([40 100])

	# %end
	# fprintf(' done\n')	


	# for i=0:(Nantb-1)
	#   fchanests(:,i+1)=[0; chanests(301*i+[1:150]); zeros(210,1); chanests(301*i+[151:301])];
	# endfor
	# tchanests=ifft(fchanests);
	
		
else
  if(!LSBSWITCH_FLAG) error("You have to set the LSB switch flag (LSBSWITCH_FLAG) in initparams.m.\n")
  else error("You have to run init.params.m first!")
  endif
endif
	   	
