function [F, HA]=alterproj(CHB2A,CHA2B, s, N, Nl, Nt)
for l=1:Nl
    YA(:,((l-1)*Nt+1):(l*Nt))=squeeze(CHB2A{l}(s,:,:));
    YB(:,((l-1)*Nt+1):(l*Nt))=squeeze(CHA2B{l}(s,:,:));
end

HA=zeros(N,Nl);
for l=1:Nl
    for t=1:Nt
        HA(:,l)=HA(:,l)+YA(:,t+(l-1)*Nt);
    end
    HA(:,l)=HA(:,l)/Nt;
end
GA=kron(HA,ones(1,Nt));
F=YB*GA'*inv(GA*GA');
oldobj=1e14;
newobj=norm(YB-F*GA)^2+norm(YA-GA)^2;

while(oldobj-newobj>1e-10)
    oldobj=newobj;
    FI=repmat([F; eye(N)],Nt,1);
    YBA=reshape([YB; YA],2*N*Nt,Nl);
    FIFI=FI'*FI;
    if(cond(FIFI)>30)
        warning(['The cond. number in HA est is ' num2str(cond(FIFI))])
    end
    HA=inv(FIFI)*FI'*YBA;
    GA=kron(HA,ones(1,Nt));
    GAGA=GA*GA';
    if(cond(GAGA)>30)
        warning(['The cond. number in F est. is '  num2str(cond(GAGA))])
    end
    F=YB*GA'*inv(GAGA);
    newobj=norm(YB-F*GA)^2+norm(YA-GA)^2;
end
end

