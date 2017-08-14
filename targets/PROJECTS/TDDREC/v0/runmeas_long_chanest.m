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
    Ntaps=8;
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
    Nmeas = 10;
    
    %% ------- Prepare the signals for A2B ---------- %%
    signalA2B=zeros(N,4,Nmeas);
    signalB2A=zeros(N,4,Nmeas);
    Da2b_T=[];
    Db2a_T=[];
    for meas=1:Nmeas
        ia=1; ib=1;
        Dtmp=[];
        for i=1:4
            if(indA(ia)==i)
                [tmpd, tmps]=genrandpskseq(N,M,amp);
                signalA2B(:,i,meas)=tmps*2; %make sure LSB is 0 (switch=tx)
                signalB2A(:,i,meas)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
                Dtmp=[Dtmp tmpd];
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
        Da2b_T=[Da2b_T; Dtmp];
        
        %%------------Prepare the signals for B2A---------------%%
        Dtmp=[];
        for i=1:4
            if(indB(ib)==i)
                [tmpd, tmps]=genrandpskseq(N,M,amp);
                signalB2A(:,i,meas)=tmps*2; %make sure LSB is 0 (switch=tx)
                signalA2B(:,i,meas)=repmat(1+1j,76800,1); %make sure LSB is 1 (switch=rx)
                Dtmp=[Dtmp tmpd];
                if(length(indB)> ib) ib=ib+1; end
            end
        end
        Db2a_T=[Db2a_T; Dtmp];
    end
    
    Da2b_R=zeros(Nmeas*120,Nantb*301);
    Db2a_R=zeros(Nmeas*120,Nanta*301);
    
    for meas=1:Nmeas
        %% ------- Node A to B transmission ------- %%
        oarf_send_frame(card,squeeze(signalA2B(:,:,meas)),n_bit);
        %keyboard
        sleep(0.01);
        receivedA2B=oarf_get_frame(card);
        %oarf_stop(card); %not good, since it does a reset
        sleep(0.01);
        
        %%----------Node B to A transmission---------%%
        oarf_send_frame(card,squeeze(signalB2A(:,:,meas)),n_bit);
        %keyboard
        sleep(0.01);
        receivedB2A=oarf_get_frame(card);
        %oarf_stop(card); %not good, since it does a reset
        
        %% ------- Do the A to B signal post preparation ------- %%
        for i=0:119;
            ifblock=receivedA2B(i*640+[1:640],indB);
            ifblock(1:128,:)=[];
            fblock=fft(ifblock);
            fblock(1,:)=[];
            fblock(151:360,:)=[];
            Da2b_R((meas-1)*120+i+1,:)=vec(fblock);
        end
        
        
        %% ------- Do the B to A signal post preparation ------- %%
        for i=0:119;
            ifblock=receivedB2A(i*640+[1:640],indA);
            ifblock(1:128,:)=[];
            fblock=fft(ifblock);
            fblock(1,:)=[];
            fblock(151:360,:)=[];
            Db2a_R((meas-1)*120+i+1,:)=fblock.';
        end
    end

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
      noise_f(i+1,:,:)=fblock;
    end
    
    %% ------- Do the A to B channel estimation ------- %%
    HA2B=repmat(conj(Da2b_T),1,Nantb).*Da2b_R;
    phasesA2B=mod(angle(HA2B),2*pi);
    if(mean(var(phasesA2B))>0.5)
        disp('The phases of your estimates from A to B are a bit high (larger than 0.5 rad.), something is wrong.');
    end
    DA2B=repmat(Da2b_T,1,Nantb);
    chanestsA2B=reshape(diag(DA2B'*Da2b_R)./diag(DA2B'*DA2B),301,Nantb);
    fchanestsA2B=[zeros(1,Nantb); chanestsA2B([1:150],:); zeros(210,Nantb); chanestsA2B(151:301,:)];
    tchanestsA2B=ifft(fchanestsA2B);
    
    %% ------- Do the B to A channel estimation ------- %%
    if (chanest_full)
      HB2A=zeros(120*Nmeas/10,301,Nantb);
      for t=1:120*Nmeas/10
        for ci=1:301;
            data=Db2a_T((t-1)*10+1:t*10,ci+[0:Nantb-1]*301);
            rec=Db2a_R((t-1)*10+1:t*10,ci);
            HB2A(t,ci,:)=(inv(data'*data)*data'*rec).';
        end
      end
      phasesB2A=mod(angle(HB2A),2*pi);
      phasesB2A=reshape(phasesB2A,[],301*Nantb);
      if(mean(var(phasesB2A))>0.5)
        disp('The phases of your estimates from B to A are a bit high (larger than 0.5 rad.), something is wrong.');
      end
    
      chanestsB2A=zeros(301,Nantb);
      for ci=1:301;
        data=Db2a_T(:,ci+[0:Nantb-1]*301);
        rec=Db2a_R(:,ci);
        chanestsB2A(ci,:)=(inv(data'*data)*data'*rec).';
      end
    else
        chanestsB2A=reshape(diag(Db2a_T'*repmat(Db2a_R,1,Nantb)/(Nmeas*60)),301,Nantb);
    end
    
    %fchanestsB2A=zeros(512,Nantb);
    %for i=1:Nantb
    %  fchanestsB2A(:,i)=[0; chanestsB2A([1:150],i); zeros(210,1); chanestsB2A(151:301,i)];
    %end
    fchanestsB2A = [zeros(1,Nantb); chanestsB2A([1:150],:); zeros(210,Nantb); chanestsB2A(151:301,:)];
    tchanestsB2A=ifft(fchanestsB2A);
    
    %% -- Some plotting code -- %%  (you can uncomment what you see fit)
    received = [receivedA2B(:,indB) receivedB2A(:,indA)];
    phases = phasesB2A;
    tchanests = [tchanestsA2B(:,:,end), tchanestsB2A(:,:,end)];
    fchanests = [chanestsA2B(:,:,end), chanestsB2A(:,:,end)];
    
    clf
    figure(1)
    for i=1:size(received,2)
        subplot(220+i);
	plot(20*log10(abs(fftshift(fft(received(:,i))))));
	ylim([20 140])
    end
    
    figure(2)
    t=[0:512-1]/512*1e-2;
    plot(t,20*log10(abs(tchanests)))
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
    Fhatloc = zeros(301,Nantb);
        for s=1:301
            ya=chanestsB2A(s,:);
            yb=chanestsA2B(s,:);
            Fhatloc(s,:)=(yb.*conj(ya))./(ya.*conj(ya));
        end
    
    figure(5)
    plot_style={'rx','go','bs'};
    hold off
    for n=1:Nantb
        plot((squeeze(Fhatloc(:,n))),plot_style{n})
        hold on
    end
    axis([-2 2 -2 2])

    %disp(squeeze(mean(Fhatloc,1)));
    drawnow
    
else
    error('You have to run init.params.m first!')
end
