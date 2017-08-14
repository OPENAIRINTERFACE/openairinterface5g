% 50 PRB
Ngap1 = 27; Ngap2= 9;NVRB1=46; NVRB2=36 ; NtildeVRB2 = 2*Ngap2; P=3; Nrow1=12; Nrow2=ceil(NtildeVRB2/(4*P))*P; Nnull=2 ; Nnull2=(4*Nrow2)-NtildeVRB2;

Nprb50_1=zeros(1,50);
Nprb50_2=zeros(1,50);
               
for i=0:49
   ntilde_VRB = rem(i,NVRB1);
   if (ntilde_VRB < (NVRB1-Nnull)  % n_PRB''
       Nprb50_1_even(i+1) = Nrow1*rem(ntilde_VRB,4) + floor(ntilde_VRB/4) + NVRB1*floor(i/NVRB1);
     if (rem(ntilde_VRB,4)>1) 
       Nprb50_1_even(i+1)= Nprb50_1_even(i+1) - (Nnull/2);
     endif
     if (Nprb50_1_even(i+1) > (NVRB1/2))
        Nprb50_1_even(i+1) = Nprb50_1_even(i+1)+Ngap1-(NVRB1/2);
     endif
   else    %n_PRB'
     Nprb50_1_even(i+1) = 2*Nrow1*rem(ntilde_VRB,2) + floor(ntilde_VRB/2) + NVRB1*floor(i/NVRB1)+(Nnull/2)*(1-rem(ntilde_VRB,2));
   endif 

   Nprb50_1_odd(i+1) = rem(Nprb50_1_even(i+1) + (NVRB1/2),NVRB1) + NVRB1*floor(i,NVRB1);

end    
   Nprb50_1(1+(46:49)) = (1:4);
   Nprb50_1(1+44) = 9;
   Nprb50_1(1+45) = 34+Ngap1-(NVRB1/2); 
   fprintf("NPRB50_1[50] = {%d",Nprb50_1(1));
   fprintf(",%d",Nprb50_1(2:end-1))
   fprintf("%d};\n",Nprb50_1(end));

for i=0:(NtildeVRB2-Nnull2-1),
   Nprb50_2(i+1) = Nrow2*rem(i,4) + floor(i/4);
   if (rem(i,4)>1) 
     Nprb50_2(i+1)= Nprb50_2(i+1) - (Nnull2/2);
   endif
   if (Nprb50_2(i+1) >= (NtildeVRB2/2))
     Nprb50_2(i+1) = Nprb50_2(i+1)+Ngap2-(NtildeVRB2/2);
   endif 
end    
   for (i=(NtildeVRB2-Nnull2):
   Nprb50_2(1+(NtildeVRB2:49)) = (1:(50-NtildeVRB2));   
   fprintf("NPRB50_2[50] = {%d",Nprb50_2(1));
   fprintf(",%d",Nprb50_2(2:end-1))
   fprintf("%d};\n",Nprb50_2(end));
