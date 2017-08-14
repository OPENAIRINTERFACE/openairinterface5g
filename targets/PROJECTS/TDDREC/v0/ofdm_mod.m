function sigt = ofdm_mod(sigf,amp)
[Nant,Nsymb,Nsub] = size(sigf);
sigt = zeros(Nant,Nsymb*640);
for i=1:Nsymb
    symbol=squeeze(sigf(:,i,:));
    % insert zero subcarriers
    symbol=cat(2,zeros(Nant,1),symbol(:,1:150),zeros(Nant,210),symbol(:,151:301));
    % ofdm modulation
    symbol_t=ifft(symbol,512,2);
    % Adding cycl. prefix making the block of 640 elements
    symbol_cp = cat(2,symbol_t(:,end-127:end), symbol_t);
    sigt(:,(1:640)+(i-1)*640)=floor(amp*symbol_cp);
end
