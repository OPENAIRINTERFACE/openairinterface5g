% Author: Mirsad Cirkic
% Organisation: Eurecom (and Linkoping University)
% E-mail: mirsad.cirkic@liu.se


function [carrierdata, s]=genrandpskseq(N,M,amp)
  
if(mod(N,640)~=0)
  error('The sequence length must be divisible with 640.');
end
s = zeros(N,1);
MPSK=exp(sqrt(-1)*([1:M]*2*pi/M+pi/M));

% OFDM sequence with 512 FFT using randomly 
% generated 4-QAM and 128 cyclic prefix
carrierdata=zeros(120,301);
for i=0:(N/640-1) 
  datablock=MPSK(ceil(rand(301,1)*M));
  for j=1:301
    carrierdata(i+1,j)=datablock(j);
  end
  fblock=[0 datablock(1:150) zeros(1,210) datablock(151:301)];
  ifblock=ifft(fblock,512)*sqrt(512);;
  % Adding cycl. prefix making the block of 640 elements	
  block = [ifblock(end-127:end) ifblock]; 
  s([1:640]+i*640)=floor(amp*block);
end

end
