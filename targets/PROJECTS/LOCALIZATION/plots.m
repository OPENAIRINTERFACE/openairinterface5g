close all  
  figure
  for i=1:4 
    subplot(220+i);plot(20*log10(abs(fftshift(fft(received(:,i))))));
    axis([0 76799 10 140])
  endfor

  figure
  t=[0:512-1]/7.68;%512*1e-2;
  plot(t,20*log10(abs(tchanests)))
  xlabel('time (usec)')
  ylabel('|h|')
  legend('SMBV->Antenna1','SMBV->Antenna2','SMBV->Antenna3','SMBV->Antenna4');
  axis([0 10 10 60])
  figure
  plot(20*log10(abs(fchanests)));
  ylim([40 100])
  xlabel('freq')
  ylabel('|h|')
  legend('SMBV->Antenna1','SMBV->Antenna2','SMBV->Antenna3','SMBV->Antenna4');

