% Author: Mirsad Cirkic, Florian Kaltenberger
% Organisation: Eurecom (and Linkoping University)
% E-mail: mirsad.cirkic@liu.se

if(paramsinitialized)
    disp(['\n\n------------\nThis code is, so far, only written for single runs. Multiple ' ...
        'runs will overwrite the previous measurement data, i.e., the ' ...
        'data structures are not defined for multiple runs. You will need to ' ...
        'add code in order to save the intermediate measurements and the ' ...
        'corresponding timestamps.\n------------'])
    N=76800;
    M=4;
    indA=find(active_rfA==1);
    indB=find(active_rfB==1);
    Nanta=length(indA);
    Nantb=length(indB);
    %if(Nanta~=1)
    %    error('Node A can only have one antenna active\n');
    %end
    Niter=1;
    if(Niter~=1)
        error('We should only use one get_frame at each run.\n');
    end
    Nmeas = 10;
    
    %% ------- Prepare the signals for A2B ---------- %%
    signalA2B=zeros(N,4);
    signalB2A=zeros(N,4);
    ia=1; ib=1;
    Da2b_T=[];
    Db2a_T=[];
    for i=1:4
        if(indA(ia)==i)
            [tmpd, tmps]=genrandpskseq(N,M,amp);
            signalA2B(:,i)=tmps*2; %make sure LSB is 0 (switch=tx)
            %signalB2A(:,i)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
	    Da2b_T = [Da2b_T tmpd];
            if(length(indA)> ia) ia=ia+1; end
        end
        %  if(indB(ib)==i)
        %   % This part could be improved by creating fully orthogonal sequences
        %   [tmpd, tmps]=genrandpskseq(N,M,amp);
        %   signalB2A(:,i)=tmps*2;
        %   signalA2B(:,i)=repmat(1+1j,76800,1);
        %   Db2a_T=[Db2a_T tmpd];
        %   if(length(indB)> ib) ib=ib+1; end
        %  end
    end
    
    %%------------Prepare the signals for B2A---------------%%
    for i=1:4
        if(indB(ib)==i)
            [tmpd, tmps]=genrandpskseq(N,M,amp);
            signalB2A(:,i)=tmps*2; %make sure LSB is 0 (switch=tx)
            %signalA2B(:,i)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
            Db2a_T=[Db2a_T tmpd];
            if(length(indB)> ib) ib=ib+1; end
        end
    end
    
    if (~chanest_full)
        signalB2A(1:38400,3)=0;
        signalB2A(38401:end,2)=0;
        Db2a_T(1:60,302:end) = 0;
        Db2a_T(61:end,1:301) = 0;
    end
    
    Da2b_R=zeros(Niter*120,Nantb*301,Nmeas);
    Db2a_R=zeros(Niter*120,Nanta*301,Nmeas);
    chanestsA2B=zeros(301,Nanta,Nantb,Nmeas);
    tchanestsA2B=zeros(512,Nanta,Nantb,Nmeas);
    fchanestsA2B=zeros(512,Nanta,Nantb,Nmeas);
    chanestsB2A=zeros(301,Nanta,Nantb,Nmeas);
    tchanestsB2A=zeros(512,Nanta,Nantb,Nmeas);
    fchanestsB2A=zeros(512,Nanta,Nantb,Nmeas);
    
    for meas=1:Nmeas
        %% ------- Node A to B transmission ------- %%
        oarf_send_frame(card,signalA2B,n_bit);
        %keyboard
        sleep(0.01);
        receivedA2B=oarf_get_frame(card);
        %oarf_stop(card); %not good, since it does a reset
        sleep(0.01);
        
        %%----------Node B to A transmission---------%%
        oarf_send_frame(card,signalB2A,n_bit);
        %keyboard
        sleep(0.01);
        receivedB2A=oarf_get_frame(card);
        %oarf_stop(card); %not good, since it does a reset
        
        %% ------- Do the A to B channel estimation ------- %%
        for i=0:119;
            ifblock=receivedA2B(i*640+[1:640],indB);
            ifblock(1:128,:)=[];
            fblock=fft(ifblock);
            fblock(1,:)=[];
            fblock(151:360,:)=[];
            Da2b_R((Niter-1)*120+i+1,:,meas)=vec(fblock);
        end

	if (chanest_full)
          for ci=1:301;
            data=Da2b_T(:,ci+[0:Nanta-1]*301);
            rec=Da2b_R(:,ci+[0:Nantb-1]*301,meas);
            chanestsA2B(ci,:,:,meas)=(inv(data'*data)*data'*rec).';
          end
	else
          chanestsA2B(:,:,meas)=reshape(diag(repmat(Da2b_T,Niter,Nantb)'*Da2b_R(:,:,meas))/size(Da2b_T,1),301,Nantb);
	end

        fchanestsA2B(:,:,:,meas)=[zeros(1,Nanta,Nantb); chanestsA2B([1:150],:,:,meas); zeros(210,Nanta,Nantb); chanestsA2B(151:301,:,:,meas)];
        tchanestsA2B(:,:,:,meas)=ifft(fchanestsA2B(:,:,:,meas));
        
        %% ------- Do the B to A channel estimation ------- %%
        for i=0:119;
            ifblock=receivedB2A(i*640+[1:640],indA);
            ifblock(1:128,:)=[];
            fblock=fft(ifblock);
            fblock(1,:)=[];
            fblock(151:360,:)=[];
            Db2a_R((Niter-1)*120+i+1,:,meas)=vec(fblock);
        end
        
        if (chanest_full)
          for ci=1:301;
            data=Db2a_T(:,ci+[0:Nantb-1]*301);
            rec=Db2a_R(:,ci+[0:Nanta-1]*301,meas);
            chanestsB2A(ci,:,:,meas)=(inv(data'*data)*data'*rec).';
          end
        else
          chanestsB2A(:,:,meas)=reshape(diag(repmat(Db2a_T,Niter,1)'*repmat(Db2a_R(:,:,meas),1,Nantb)/(Niter*60)),301,Nantb);
        end
        

        fchanestsB2A(:,:,:,meas) = [zeros(1,Nanta,Nantb); chanestsB2A([1:150],:,:,meas); zeros(210,Nanta,Nantb); chanestsB2A(151:301,:,:,meas)];
        tchanestsB2A(:,:,:,meas)=ifft(fchanestsB2A(:,:,:,meas));
    end
    
    %% -- Some plotting code -- %%  (you can uncomment what you see fit)
    received = [receivedB2A(:,indA) receivedA2B(:,indB)];
    tchanests = [tchanestsA2B(:,:,end), tchanestsB2A(:,:,end)];
    fchanests = [fchanestsA2B(:,:,end), fchanestsB2A(:,:,end)];
    
    clf
    if (0)
    figure(1)
    for i=1:size(received,2);
        subplot(220+i);
	plot(20*log10(abs(fftshift(fft(received(:,i))))));
	ylim([20 140])
    end
    end

    figure(2)
    t=[0:512-1]/512*1e-2;
    plot(t,20*log10(abs(tchanests)))
    xlabel('time')
    ylabel('|h|')
    legend('A->B1','A->B2','A->B3','B1->A','B2->A','B3->A');
    %legend('A->B1','A->B2','B1->A','B2->A');
    
    figure(3)
    plot(20*log10(abs(fchanests)));
    ylim([40 100])
    xlabel('freq')
    ylabel('|h|')
    legend('A->B1','A->B2','A->B3','B1->A','B2->A','B3->A');
    %legend('A->B1','A->B2','B1->A','B2->A');
    
    if (0)
        figure(4)
        wndw = 50;
        for i=1:5:Nantb*301             %# sliding window size
            phamean = filter(ones(wndw,1)/wndw, 1, phases(:,i)); %# moving average
            plot(phamean(wndw:end),'LineWidth',2);
            title(['subcarrier ' num2str(i)]);
            xlabel('time')
            ylabel('phase')
            ylim([-pi pi])
            drawnow;
            pause(0.1)
        end
        phavar=var(phases);
        plotphavar=[];
        for i=0:Nantb-1
            plotphavar=[plotphavar; phavar([1:301]+i*301)];
        end
        plot([1:150 362:512],plotphavar,'o');
        %ylim([0 pi])
        xlabel('subcarrier')
        ylabel('phase variance')
    end
    
    figure(4);
    for t=1
      i=1;
      for m=1:Nanta
	for n=1:Nantb
	  subplot(Nanta,Nantb,i);
	  plot(20*log10(abs(fchanestsB2A(:,m,n,t))))
	  hold on
	  plot(20*log10(abs(fchanestsA2B(:,m,n,t))),'r')
	  ylim([0 80])
	  i=i+1;
	end
      end
    end
    
    figure(5)
    for t=1
      for m=1:Nanta
	  subplot(2,4,m);
	  Rb2a_comp = repmat(chanestsB2A(:,m,m,t)',120,1).*Db2a_R(:,(m-1)*301+(1:301),t);
	  plot(Rb2a_comp(:),'x');
	  subplot(2,4,4+m);
	  Ra2b_comp = repmat(chanestsA2B(:,m,m,t)',120,1).*Da2b_R(:,(m-1)*301+(1:301),t);
	  plot(Ra2b_comp(:),'rx');
      end
    end

    if (0)
    %% estimate F matrix assuming it is diagonal for sanity checking
    Fhatloc = zeros(Nmeas,301,Nanta,Nantb);
    for t=1:Nmeas
        for s=1:301
	  for m=1:Nanta
	    for n=1:Nantb
              ya=chanestsB2A(s,m,n,t);
              yb=chanestsA2B(s,m,n,t);
              Fhatloc(t,s,m,n)=(yb.*conj(ya))./(ya.*conj(ya));
	    end
	  end
        end
    end
    
    figure(5)
    plot_style={'rx','go','bs','kd'};
    hold off
    for m=1:Nanta
    for n=1:Nantb
        plot((squeeze(Fhatloc(:,:,m,n))),plot_style{m+(n-1)*2})
        hold on
    end
    end
    axis([-2 2 -2 2])

    %disp(squeeze(mean(Fhatloc,2)));
    end

else
  error('You have to run init.params.m first!')
end
