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
    if(Nanta~=1)
        error('Node A can only have one antenna active\n');
    end
    Niter=1;
    if(Niter~=1)
        error('We should only use one get_frame at each run.\n');
    end
    Nmeas = 5;
    
    %% ------- Prepare the signals for A2B ---------- %%
    signalA2B=zeros(N,4);
    signalB2A=zeros(N,4);
    ia=1; ib=1;
    Db2a_T=[];
    for i=1:4
        if(indA(ia)==i)
            [Da2b_T, tmps]=genrandpskseq(N,M,amp);
            signalA2B(:,i)=tmps*2; %make sure LSB is 0 (switch=tx)
            signalB2A(:,i)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
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
            signalA2B(:,i)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
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
    chanestsA2B=zeros(301,Nantb,Nmeas);
    tchanestsA2B=zeros(512,Nantb,Nmeas);
    fchanestsA2B=zeros(512,Nantb,Nmeas);
    chanestsB2A=zeros(301,Nantb,Nmeas);
    tchanestsB2A=zeros(512,Nantb,Nmeas);
    fchanestsB2A=zeros(512,Nantb,Nmeas);
    
    for meas=1:Nmeas
        %% ------- Node A to B transmission ------- %%
        oarf_send_frame(card,signalA2B,n_bit);
        receivedA2B=oarf_get_frame(card);
        %oarf_stop(card); %not good, since it does a reset
        sleep(0.01);
        
        %%----------Node B to A transmission---------%%
        oarf_send_frame(card,signalB2A,n_bit);

        receivedB2A=oarf_get_frame(card);
        %oarf_stop(card); %not good, since it does a reset
        
        %keyboard;

        %% ------- Do the A to B channel estimation ------- %%
        for i=0:119;
            ifblock=receivedA2B(i*640+[1:640],indB);
            ifblock(1:128,:)=[];
            fblock=fft(ifblock);
            fblock(1,:)=[];
            fblock(151:360,:)=[];
            Da2b_R((Niter-1)*120+i+1,:,meas)=vec(fblock);
        end
        HA2B=repmat(conj(Da2b_T),Niter,Nantb).*Da2b_R(:,:,meas);
        phasesA2B=unwrap(angle(HA2B));
        if(mean(var(phasesA2B))>0.5)
            disp('The phases of your estimates from A to B are a bit high (larger than 0.5 rad.), something is wrong.');
        end
        chanestsA2B(:,:,meas)=reshape(diag(repmat(Da2b_T,Niter,Nantb)'*Da2b_R(:,:,meas))/size(Da2b_T,1),301,Nantb);
        %fchanestsA2B=zeros(512,Nantb);
        %for i=1:Nantb
        %  fchanestsA2B(:,i)=[0; chanestsA2B([1:150],i,meas); zeros(210,1); chanestsA2B(151:301,i,meas)];
        %end
        fchanestsA2B(:,:,meas)=[zeros(1,Nantb); chanestsA2B([1:150],:,meas); zeros(210,Nantb); chanestsA2B(151:301,:,meas)];
        tchanestsA2B(:,:,meas)=ifft(fchanestsA2B(:,:,meas));
        
        %% ------- Do the B to A channel estimation ------- %%
        for i=0:119;
            ifblock=receivedB2A(i*640+[1:640],indA);
            ifblock(1:128,:)=[];
            fblock=fft(ifblock);
            fblock(1,:)=[];
            fblock(151:360,:)=[];
            Db2a_R((Niter-1)*120+i+1,:,meas)=fblock.';
        end
        HB2A=conj(repmat(Db2a_T,Niter,1)).*repmat(Db2a_R(:,:,meas),1,Nantb);
        phasesB2A=unwrap(angle(HB2A));
        %if(mean(var(phasesB2A))>0.5)
        %    disp('The phases of your estimates from B to A are a bit high (larger than 0.5 rad.), something is wrong.');
        %end
        
        if (chanest_full)
            chanestsB2A(:,:,meas)=zeros(301,Nantb);
            inds=repmat([1:Nantb]',1,301);
            for ci=1:301;
                data=Db2a_T(:,ci+[0:Nantb-1]*301);
                rec=Db2a_R(:,ci,meas);
                chanestsB2A(ci,:,meas)=(inv(data'*data)*data'*rec).';
            end
        else
            chanestsB2A(:,:,meas)=reshape(diag(repmat(Db2a_T,Niter,1)'*repmat(Db2a_R(:,:,meas),1,Nantb)/(Niter*60)),301,Nantb);
        end
        
        %fchanestsB2A=zeros(512,Nantb);
        %for i=1:Nantb
        %  fchanestsB2A(:,i)=[0; chanestsB2A([1:150],i); zeros(210,1); chanestsB2A(151:301,i)];
        %end
        fchanestsB2A(:,:,meas) = [zeros(1,Nantb); chanestsB2A([1:150],:,meas); zeros(210,Nantb); chanestsB2A(151:301,:,meas)];
        tchanestsB2A(:,:,meas)=ifft(fchanestsB2A(:,:,meas));
    end % Nmeas

    %% estimate the noise
    no_signal=repmat(1+1j,76800,4);
    oarf_send_frame(card,no_signal,n_bit);
    sleep(0.01);
    noise_received=oarf_get_frame(card);
    % estimate noise in frequency domain
    noise_f = zeros(120,301,4);
    for i=0:119;
      ifblock=noise_received(i*640+[1:640],:);
      ifblock(1:128,:)=[];
      fblock=fft(ifblock);
      fblock(1,:)=[];
      fblock(151:360,:)=[];
      %noise_f(i+1,:,:)=fblock;
    end


    %% -- Some plotting code -- %%  (you can uncomment what you see fit)
    received = [receivedA2B(:,indB) receivedB2A(:,indA)];
    phases = phasesB2A;
    tchanests = [tchanestsA2B(:,:,end), tchanestsB2A(:,:,end)];
    fchanests = [chanestsA2B(:,:,end), chanestsB2A(:,:,end)];
    
    clf
    figure(1)
    for i=1:size(received,2);
        subplot(220+i);
	plot(20*log10(abs(fftshift(fft(received(:,i))))));
	ylim([20 140])
    end
    
    figure(2)
    %t=[0:512-1]/512*1e-2;
    %plot(t,20*log10(abs(tchanests)))
    plot(20*log10(abs(tchanests)))
    xlabel('time')
    ylabel('|h|')
    if Nantb==3
      legend('A->B1','A->B2','A->B3','B1->A','B2->A','B3->A');
    else
      legend('A->B1','A->B2','B1->A','B2->A');
    end

    figure(3)
    plot(20*log10(abs(fchanests)));
    hold on
    plot(squeeze(10*log10(mean(abs(noise_f(:,:,[indB indA])).^2,1))),'.');
    hold off
    ylim([40 100])
    xlabel('freq')
    ylabel('|h|')
    if Nantb==3
      legend('A->B1','A->B2','A->B3','B1->A','B2->A','B3->A','Noise B1','Noise B2','Noise B3','Noise A');
    else
      legend('A->B1','A->B2','B1->A','B2->A','Noise B1','Noise B2','Noise A');
    end
    
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
    
    
    %% estimate F matrix assuming it is diagonal for sanity checking
    Fhatloc = zeros(Nmeas,301,Nantb);
    for t=1:Nmeas
        for s=1:301
            ya=chanestsB2A(s,:,t);
            yb=chanestsA2B(s,:,t);
            Fhatloc(t,s,:)=(yb.*conj(ya))./(ya.*conj(ya));
        end
    end
    
    figure(5)
    plot_style={'rx','go','bs'};
    hold off
    for n=1:Nantb
        plot((squeeze(Fhatloc(:,:,n))),plot_style{n})
        hold on
    end
    axis([-2 2 -2 2])

    %disp(squeeze(mean(Fhatloc,2)));
    drawnow;
    
else
  error('You have to run init.params.m first!')
end
