filter_length = 12;

F = -3/4:1/4:7/4;
F_l = zeros(8,12);
F_r = zeros(8,12);
F_m = zeros(8,12);

F2 =-3/5:1/5:8/5;

for i=0:3
  F_l(i+1,:)  = floor(16384*[F(8+i:-1:4) zeros(1,7-i)]);
  F_r(i+1,:)  = floor(16384*[zeros(1,4+i) F(4:end-i)]);
  F_m(i+1,:)  = floor(16384*[F(4-i:8) F(7:-1:1+i)]);
end

for i=0:1
  F_l(i+5,:)  = floor(16384*[F(8:-1:4-i) zeros(1,7-i)]);
  F_r(i+5,:)  = floor(16384*[zeros(1,5+i) F2(5+i) F2(7:end-i)]);
  F_m(i+5,:)  = floor(16384*[F(4-i:8) F2(8-i) F2(6:-1:1+i)]);
end

for i=2:3
  F_l(i+5,:)  = floor(16384*[F2(end:-1:7) F2(8-i) zeros(1,5)]);
  F_r(i+5,:)  = floor(16384*[zeros(1,4+i) F(4:end-i)]);
  F_m(i+5,:)  = floor(16384*[F2(4-i:6) F2(4+i) F(8:-1:1+i)]);
end


fd = fopen("filt12_32.h","w");

for i=0:3
  fprintf(fd,"short filt12_l%d[12] = {\n",i);
  fprintf(fd,"%d,",F_l(i+1,1:end-1));
  fprintf(fd,"%d};\n\n",F_l(i+1,end));
  
  fprintf(fd,"short filt12_r%d[12] = {\n",i);
  fprintf(fd,"%d,",F_r(i+1,1:end-1));
  fprintf(fd,"%d};\n\n",F_r(i+1,end));
  
  fprintf(fd,"short filt12_m%d[12] = {\n",i);
  fprintf(fd,"%d,",F_m(i+1,1:end-1));
  fprintf(fd,"%d};\n\n",F_m(i+1,end));
end

for i=0:3
  fprintf(fd,"short filt12_l%d_dc[12] = {\n",i);
  fprintf(fd,"%d,",F_l(i+5,1:end-1));
  fprintf(fd,"%d};\n\n",F_l(i+5,end));
  
  fprintf(fd,"short filt12_r%d_dc[12] = {\n",i);
  fprintf(fd,"%d,",F_r(i+5,1:end-1));
  fprintf(fd,"%d};\n\n",F_r(i+5,end));
  
  fprintf(fd,"short filt12_m%d_dc[12] = {\n",i);
  fprintf(fd,"%d,",F_m(i+5,1:end-1));
  fprintf(fd,"%d};\n\n",F_m(i+5,end));
end

fclose(fd);
