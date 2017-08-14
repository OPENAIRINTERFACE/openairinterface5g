chan=1; 
hold off ; 
card=0;
s=oarf_get_frame(card);

s2=zeros(76800*2,1);
s2(1:2:end) = s(:,chan);
s2(2:2:end) = s(:,chan);
 
plot(real(s2),'b',"markersize",1);  
hold on; plot(imag(s2),'r',"markersize",1); 

bit11_I = rem(floor(real(2048+s(:,chan))/2048),2);
bit11_Q = rem(floor(imag((2048*j)+s(:,chan))/2048),2);
bit11_IQ = zeros(1,2*length(bit11_I));
bit11_IQ(1:2:end) = bit11_I;
bit11_IQ(2:2:end) = bit11_Q;
plot(500*bit11_IQ,'k')
axis([0 76800 -1000 1000])
