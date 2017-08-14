vlen=76800
chan=2

gainimb_rx = -0.0020;  phaseimb_rx = -2.38; % ExMIMO1 / lime1, VGAgain2 = 0, 1.9 GHz
gainimb_rx = 0.55;  phaseimb_rx = 24.3; % ExMIMO1 / lime1, VGAgain2 = 0, 1.9 GHz

phaseimb_rx = phaseimb_rx/180*pi; % phaser imb in radians
beta_rx = (1/2)*(1 + (1+ gainimb_rx) * exp(1i*phaseimb_rx));
alpha_rx = (1/2)*(1 - (1+ gainimb_rx) * exp(-1i*phaseimb_rx));
den=abs(beta_rx)^2-abs(alpha_rx)^2;
beta_rx=beta_rx/den;
alpha_rx=alpha_rx/den;

s2 =  beta_rx.*s + alpha_rx.*conj(s); 
 hold off ; plot(20*log10(abs(fftshift(fft(s2(:,chan)))))); grid on;


