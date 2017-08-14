clear all
j=1;
for alpha=0:5,
for beta=0:4,
for gamma=0:3,

dftsize(j) = 12 * (2^alpha) * (3^beta) * (5^gamma);
if (dftsize(j)<=1200)
  printf("dftsize %d, 2^%d 3^%d 5 ^%d\n",dftsize(j),alpha,beta,gamma);
  j=j+1;
endif

end
end
end

dftsize = sort(dftsize);