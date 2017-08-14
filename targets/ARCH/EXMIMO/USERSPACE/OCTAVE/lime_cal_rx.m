clear all
close all
card=0;

limeparms;
freq_rx = 1907600000*[1 1 1 1];
%freq_rx = 700600000*[1 1 1 1];
%freq_rx = 748000000*[1 1 1 1];
freq_tx = freq_rx+1920000;
rxgain = 30*[1 1 1 1];
txgain = 0*[1 1 1 1];
tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_TESTRX;
syncmode = SYNCMODE_FREE;
autocal = [1 1 1 1];
resampling = [2 2 2 2];

rf_mode = (RXEN+0+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM+DMAMODE_RX+0) * [1 1 1 1];
rf_local = [8254813 8255016 8254813 8254813]; % from the tx calibration
%rf_local = [8255842   8255064   8257340   8257340]; % 700MHz
%rf_local = [8256776   8255788   8257340   8257340]; % 850MHz
rf_rxdc  = rf_rxdc*[1 1 1 1]; % initial value
rf_vcocal= rf_vcocal_19G*[1 1 1 1];
rffe_rxg_low = 31*[1 1 1 1];
rffe_rxg_final = 31*[1 1 1 1];
rffe_band = B19G_TDD*[1 1 1 1];

oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,0,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling);

autocal = [2 2 2 2];
sleepafterconfig=0.2

% coarse calibration loop for both rx chains
for ant=1:4
%rf_mode = zeros(1,4);
%rf_mode(ant) = (RXEN+0+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAMax+RFBBNORM+DMAMODE_RX+0);
dc_off_re_min = 99999;
dc_off_im_min = 99999;

stepsize = 64;
rxdc_Q_s = 0;
rxdc_I = 0;
while (stepsize>=1)
        stepsize = stepsize/2;

        if (rxdc_I<0)
	  rxdc_I_s = 64+abs(rxdc_I);
	else
	  rxdc_I_s = rxdc_I;	  
	end
        rf_rxdc(ant)   = ((128+rxdc_I_s) + (128+rxdc_Q_s)*(2^8));

	oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,0,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling);
	sleep(sleepafterconfig)

        s=oarf_get_frame(card);
	figure(1)
	plot(real(s));
	ylim([-2048 2048]);
	drawnow;
        %sF0 = 20*log10(abs(fftshift(fft(s(:,1)))));
        %sF1 = 20*log10(abs(fftshift(fft(s(:,2)))));
	%f = (7.68*(0:length(s(:,1))-1)/(length(s(:,1))))-3.84;
        %spec0 = sF0;
        %spec1 = sF1;

	rxdc_I
	rxdc_I_s
	dc_offset = mean(s(:,ant));
        fflush(stdout);

	if (abs(real(dc_offset))<dc_off_re_min)
	  dc_off_re_min = abs(real(dc_offset));
	  rxdc_I_min(ant) = rxdc_I_s;
	end

        if (real(dc_offset)>0)
	  rxdc_I = rxdc_I+stepsize;
	else
	  rxdc_I = rxdc_I-stepsize;
        end
end

stepsize = 64;
rxdc_Q = 0;
while (stepsize>=1)
        stepsize = stepsize/2;

        if (rxdc_Q<0)
	  rxdc_Q_s = 64+abs(rxdc_Q);
	else
	  rxdc_Q_s = rxdc_Q;	  
	end
        rf_rxdc(ant)   = ((128+rxdc_I_s) + (128+rxdc_Q_s)*(2^8));

	oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,0,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band,autocal,resampling);
	sleep(sleepafterconfig)

        s=oarf_get_frame(card);
	figure(2)
	plot(imag(s));
	ylim([-2048 2048]);
	drawnow;
        %sF0 = 20*log10(abs(fftshift(fft(s(:,1)))));
        %sF1 = 20*log10(abs(fftshift(fft(s(:,2)))));
	%f = (7.68*(0:length(s(:,1))-1)/(length(s(:,1))))-3.84;
        %spec0 = sF0;
        %spec1 = sF1;


	rxdc_Q
	rxdc_Q_s
	dc_offset = mean(s(:,ant));
        fflush(stdout);

	if (abs(imag(dc_offset))<dc_off_im_min)
	  dc_off_im_min = abs(imag(dc_offset));
	  rxdc_Q_min(ant) = rxdc_Q_s;
	end

        if (imag(dc_offset)>0)
	  rxdc_Q = rxdc_Q+stepsize;
	else
	  rxdc_Q = rxdc_Q-stepsize;
        end
end
end

rxdc_I_min
rxdc_Q_min

rf_rxdc_min   = ((128+rxdc_I_min) + (128+rxdc_Q_min)*(2^8))