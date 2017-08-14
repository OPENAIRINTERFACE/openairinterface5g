% generate s
x = zeros(1,30);
x(1:5) = [0 0 0 0 1];
for i=1:26
  x(i+5) = rem(x(i+2) + x(i),2);
end
s = 1 - 2*x;

% generate z
x = zeros(1,30);
x(1:5) = [0 0 0 0 1];
for i=1:26
  x(i+5) = rem(x(i+4)+ x(i+2) + x(i+1) + x(i),2);
end
z = 1 - 2*x;
% generate c
x = zeros(1,30);
x(1:5) = [0 0 0 0 1];
for i=1:26
  x(i+5) = rem(x(i+3) + x(i),2);
end
c = 1 - 2*x;
d0 = zeros(504,62);
d5 = zeros(504,62);
for Nid2=0:2,
  c0 = [c((1+Nid2) : end) c(1:Nid2)];
  c1 = [c((4+Nid2) : end) c(1:(3+Nid2))];
  for Nid1=0:167,
    qprime = floor(Nid1/30);
    q      = floor((Nid1+qprime*(qprime+1)/2)/30);
    mprime = Nid1 + q*(q+1)/2;
    m0 = rem(mprime,31);
    m1 = rem(m0+floor(mprime/31)+1,31);
    sm0 = [s((1+m0) : end) s(1:m0)];
    sm1 = [s((1+m1) : end) s(1:m1)];
    m0mod8 = rem(m0,8);
    m1mod8 = rem(m1,8);
    zm0 = [z((1+m0mod8) : end) z(1:m0mod8)];
    zm1 = [z((1+m1mod8) : end) z(1:m1mod8)];
    d0(1+Nid2+(3*Nid1),1:2:62) = sm0.*c0;
    d5(1+Nid2+(3*Nid1),1:2:62) = sm1.*c0;
    d0(1+Nid2+(3*Nid1),2:2:62) = sm1.*c1.*zm0;
    d5(1+Nid2+(3*Nid1),2:2:62) = sm0.*c1.*zm1;
 end
end
fd = fopen("sss.h","w");
fprintf(fd,"s16 d0_sss[504*62] = {");
fprintf(fd,"%d,",d0);
fprintf(fd,"};\n\n");
fprintf(fd,"s16 d5_sss[504*62] = {");
fprintf(fd,"%d,",d5);
fprintf(fd,"};\n\n");
fclose(fd);