# % Author: Mirsad Cirkic
# % Organisation: Eurecom (and Linkoping University)
# % E-mail: mirsad.cirkic@liu.se

function [carrierdata, s]=genorthqpskseq(Ns,N,amp)
  
if(N!=512*150)
  error('The sequence length must be 76800.');
endif

s = zeros(N,Ns);
H=1; for k=1:log2(128) H=[H H; H -H]; end; H=H(:,1:120);
i=1; while(i<size(H,1)) h=H(i,:); inds=find(h*H'!=0); H(inds,:)=[]; H=[h; H]; i=i+1; end
Hc=H+sqrt(-1)*H;
if(sum(Hc*Hc'-240*eye(8))>0) error("The code is not orhtogonal\n"); endif

carrierdata=zeros(120,Ns*301);

inds=1:8;
ind=8;
for i=1:301
  for k=1:Ns    
    carrierdata(:,i+(k-1)*301)=Hc(inds(ind),:)';    
    inds(ind)=[];
    ind=ind-1;
    if(ind==0) 
      inds=1:8;
      ind=8;
    endif
  endfor
endfor

for k=1:Ns  
  frstgroup=(k-1)*301+[1:150]; # The first group of OFDM carriers
  sndgroup=(k-1)*301+[151:301]; # The second group of OFDM carriers
  for i=0:119
    fblock=[0 carrierdata(i+1,frstgroup) zeros(1,210) carrierdata(i+1,sndgroup)];
    ifblock=ifft(fblock,512)*sqrt(512);				
    block = [ifblock(end-127:end) ifblock]; # Cycl. prefix 
    s([1:640]+i*640,k)=floor(amp*block);
  endfor
endfor

endfunction