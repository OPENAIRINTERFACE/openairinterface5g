clear;
card=0;

limeparms;
TX_LO_LEAKAGE_MIN0 = 99999;
TX_LO_LEAKAGE_MIN1 = 99999;
freq_rx = 1907600000*[1 1 1 1];
%freq_rx = 859500000*[1 1 1 1];
freq_tx = freq_rx+1920000;
rf_vcocal = rf_vcocal_19G*[1 1 1 1];
rf_rxdc   = rf_rxdc*[1 1 1 1];
rxgain = 30*[1 1 1 1];
txgain = 25*[1 1 1 1];
tdd_config = DUPLEXMODE_FDD + TXRXSWITCH_LSB;
syncmode = SYNCMODE_FREE;
rffe_rxg_low = 31*[1 1 1 1];
rffe_rxg_final = 31*[1 1 1 1];
rffe_band = TVWS_TDD*[1 1 1 1];

sleepafterconfig=0.2

% coarse calibration loop for both rx chains
for txdc_I=0:4:63
  for txdc_Q=0:4:63
        rf_mode   = (RXEN+TXEN+TXLPFNORM+TXLPFEN+TXLPF25+RXLPFNORM+RXLPFEN+RXLPF25+LNA1ON+LNAByp+RFBBLNA1)*[1 1 0 0];
        rf_mode = rf_mode + (DMAMODE_RX+DMAMODE_TX)*[1 1 0 0];
        rf_local  = (txdc_I + (txdc_Q)*(2^6) + 31*(2^12) + 31*(2^18))*[1 1 1 1];

	oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,0,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band);
	sleep(sleepafterconfig)
	
        s=oarf_get_frame(card);
        sF0 = 20*log10(abs(fftshift(fft(s(:,1)))));
        sF1 = 20*log10(abs(fftshift(fft(s(:,2)))));
	
        f = (7.68*(0:length(s(:,1))-1)/(length(s(:,1))))-3.84;
        spec0 = sF0;
        spec1 = sF1;

        RX_LO_LEAKAGE0 = sF0(1+(length(sF0)/2));
        TX_LO_LEAKAGE0 = sF0(1+(.75*length(sF0)));
        TX_SIG0        = sF0(1+((11/16)*length(sF0)));
        TX_IQ0         = sF0(1+((13/16)*length(sF0))) - TX_SIG0;
        RX_IQ0         = sF0(1+((5/16)*length(sF0))) - TX_SIG0;
        RX_LO_LEAKAGE1 = sF1(1+(length(sF0)/2));
        TX_LO_LEAKAGE1 = sF1(1+(.75*length(sF0)));
        TX_SIG1        = sF1(1+((11/16)*length(sF0)));
        TX_IQ1         = sF1(1+((13/16)*length(sF0))) - TX_SIG1;
        RX_IQ1         = sF1(1+((5/16)*length(sF0))) - TX_SIG1;
%       subplot(211)
%       plot(f',spec0,'r')
%       axis([-3.84,3.84,40,160])
%       subplot(212)
%       plot(f',spec1,'b')
%       axis([-3.84,3.84,40,160])
%       grid
%       drawnow
        if (TX_LO_LEAKAGE0 < TX_LO_LEAKAGE_MIN0)
          TX_LO_LEAKAGE_MIN0=TX_LO_LEAKAGE0;
          txdc_I_min0 = txdc_I;
          txdc_Q_min0 = txdc_Q;
          TX_LO_LEAKAGE0
          txdc_I_min0
          txdc_Q_min0
          fflush(stdout);
        end
        if (TX_LO_LEAKAGE1 < TX_LO_LEAKAGE_MIN1)
          TX_LO_LEAKAGE_MIN1=TX_LO_LEAKAGE1;
          txdc_I_min1 = txdc_I;
          txdc_Q_min1 = txdc_Q;
          TX_LO_LEAKAGE1
          txdc_I_min1
          txdc_Q_min1
          fflush(stdout);
        end        
  end
end

% fine calibration loop for RX1 (RX2 stays constant)
txdc_I_min02 = txdc_I_min0;
txdc_Q_min02 = txdc_Q_min0;
for deltaI = -3:3,
  for deltaQ = -3:3,
     txdc_I = txdc_I_min0 + deltaI;
     txdc_Q = txdc_Q_min0 + deltaQ;

     if (txdc_I>=0) && (txdc_Q>=0) && (txdc_I < 64) && (txdc_Q < 64)
%     printf("txdc_IQ => (%d,%d)\n",txdc_I,txdc_Q);
%     fflush(stdout);


     rf_local(1)  = txdc_I + (txdc_Q)*(2^6) + 31*(2^12) + 31*(2^18); 
     rf_local(2)  = txdc_I_min1 + (txdc_Q_min1)*(2^6) + 31*(2^12) + 31*(2^18);

     oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,0,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band);
     sleep(sleepafterconfig)

     s=oarf_get_frame(card);
     sF0 = 20*log10(abs(fftshift(fft(s(:,1)))));
     spec0 = sF0;
     spec1 = sF1;

     RX_LO_LEAKAGE = sF0(1+(length(sF0)/2));
     TX_LO_LEAKAGE = sF0(1+(.75*length(sF0)));
     TX_SIG        = sF0(1+((11/16)*length(sF0)));
     TX_IQ         = sF0(1+((13/16)*length(sF0))) - TX_SIG;
     RX_IQ         = sF0(1+((5/16)*length(sF0))) - TX_SIG;
%    subplot(211)
%    plot(f',spec0,'r')
%    axis([-3.84,3.84,40,160])
%    grid
%    drawnow
     if (TX_LO_LEAKAGE < TX_LO_LEAKAGE_MIN0)
        TX_LO_LEAKAGE_MIN0=TX_LO_LEAKAGE;
        txdc_I_min02 = txdc_I;
        txdc_Q_min02 = txdc_Q;
        TX_LO_LEAKAGE
        txdc_I_min02
        txdc_Q_min02
	fflush(stdout);
     end   
     end
  end
end

% fine calibration loop for RX2 (RX1 stays constant)
txdc_I_min12 = txdc_I_min1;
txdc_Q_min12 = txdc_Q_min1;
for deltaI = -3:3,
  for deltaQ = -3:3,
    txdc_I = txdc_I_min1 + deltaI;
    txdc_Q = txdc_Q_min1 + deltaQ;

    if (txdc_I>=0) && (txdc_Q>=0) && (txdc_I < 64) && (txdc_Q < 64)
 %     printf("txdc_IQ => (%d,%d)\n",txdc_I,txdc_Q);
 %     fflush(stdout);

%      rf_local  = txdc_I_min02 + (txdc_Q_min02)*(2^6) + (txdc_I)*(2^12) + (txdc_Q)*(2^18);
      rf_local(1)  = txdc_I_min02 + (txdc_Q_min02)*(2^6) + 31*(2^12) + 31*(2^18);
      rf_local(2)  = txdc_I + (txdc_Q)*(2^6) + 31*(2^12) + 31*(2^18);

      oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,0,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band);
      sleep(sleepafterconfig)

      s=oarf_get_frame(card);
      sF1 = 20*log10(abs(fftshift(fft(s(:,2)))));
      spec1 = sF1;

      RX_LO_LEAKAGE = sF1(1+(length(sF0)/2));
      TX_LO_LEAKAGE = sF1(1+(.75*length(sF0)));
      TX_SIG        = sF1(1+((11/16)*length(sF0)));
      TX_IQ         = sF1(1+((13/16)*length(sF0))) - TX_SIG;
      RX_IQ         = sF1(1+((5/16)*length(sF0))) - TX_SIG;
%     subplot(212)
%     plot(f',spec1,'b')
%     axis([-3.84,3.84,40,160])
%     grid
%     drawnow

      if (TX_LO_LEAKAGE < TX_LO_LEAKAGE_MIN1)
        TX_LO_LEAKAGE_MIN1=TX_LO_LEAKAGE;
        txdc_I_min12 = txdc_I;
        txdc_Q_min12 = txdc_Q;
        TX_LO_LEAKAGE
        txdc_I_min12
        txdc_Q_min12
        fflush(stdout);
      end   
    end
  end
end

TX_LO_LEAKAGE_MIN0
txdc_I_min02
txdc_Q_min02
TX_LO_LEAKAGE_MIN1
txdc_I_min12
txdc_Q_min12

rf_local(1)  = txdc_I_min02 + (txdc_Q_min02)*(2^6) + 31*(2^12) + 31*(2^18);
rf_local(2)  = txdc_I_min12 + (txdc_Q_min12)*(2^6) + 31*(2^12) + 31*(2^18);


oarf_config_exmimo(card, freq_rx,freq_tx,tdd_config,syncmode,rxgain,txgain,0,rf_mode,rf_rxdc,rf_local,rf_vcocal,rffe_rxg_low,rffe_rxg_final,rffe_band);
sleep(sleepafterconfig)

s=oarf_get_frame(card);
sF0 = 20*log10(abs(fftshift(fft(s(:,1)))));
sF1 = 20*log10(abs(fftshift(fft(s(:,2)))));
spec0 = sF0;
spec1 = sF1;

RX_LO_LEAKAGE = sF0(1+(length(sF0)/2))
TX_LO_LEAKAGE = sF0(1+(.75*length(sF0)))
TX_SIG        = sF0(1+((11/16)*length(sF0)))
TX_IQ         = sF0(1+((13/16)*length(sF0))) - TX_SIG
RX_IQ         = sF0(1+((5/16)*length(sF0))) - TX_SIG

RX_LO_LEAKAGE = sF1(1+(length(sF0)/2))
TX_LO_LEAKAGE = sF1(1+(.75*length(sF0)))
TX_SIG        = sF1(1+((11/16)*length(sF0)))
TX_IQ         = sF1(1+((13/16)*length(sF0))) - TX_SIG
RX_IQ         = sF1(1+((5/16)*length(sF0))) - TX_SIG

subplot(211)
plot(f',spec0,'r')
subplot(212)
plot(f',spec1,'b')
  axis([-3.84,3.84,40,160])
  grid
  drawnow

  %gpib_send(gpib_card,gpib_device,'OUTP:STAT OFF'); %  activate output 
  %legend('Antenna Port 0','Antenna Port 1');
grid

rf_local

