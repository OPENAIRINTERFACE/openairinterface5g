channel_length = 128;
factor = (1.5);
filter_length = 96;

F = real(fft([ones(1,1+channel_length/2) zeros(1,2048-channel_length-1) ones(1,channel_length/2)])/sqrt(2048));

 
Ftrunk0  = floor(32767*[F((2049-filter_length/2):2048) F(1:filter_length/2)]);
Ftrunk1  = floor(32767*[F((2048-filter_length/2):2048) F(1:-1+filter_length/2)]);
Ftrunk2  = floor(32767*[F((2047-filter_length/2):2048) F(1:-2+filter_length/2)]);
Ftrunk3  = floor(32767*[F((2046-filter_length/2):2048) \
			F(1:-3+filter_length/2)]);
Ftrunk4  = floor(32767*[F((2045-filter_length/2):2048) \
			F(1:-4+filter_length/2)]);
Ftrunk5  = floor(32767*[F((2044-filter_length/2):2048) \
			F(1:-5+filter_length/2)]);
Ftrunk6  = floor(32767*[F((2043-filter_length/2):2048) \
			F(1:-6+filter_length/2)]);
Ftrunk7  = floor(32767*[F((2042-filter_length/2):2048) \
			F(1:-7+filter_length/2)]);
Ftrunk8  = floor(32767*[F((2041-filter_length/2):2048) F(1:-8+filter_length/2)]);

[a,b] = max(Ftrunk0)
[a,b] = max(Ftrunk1)
[a,b] = max(Ftrunk2)
[a,b] = max(Ftrunk3)
[a,b] = max(Ftrunk4)
[a,b] = max(Ftrunk5)

plot(Ftrunk0,'r',Ftrunk2,'b')

fd = fopen("filt96_32.h","w");
fprintf(fd,"short filt96_32_0[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk0(1:length(Ftrunk0)-1)));
fprintf(fd,"%d};\n",Ftrunk0(end));

fprintf(fd,"short filt96_32_1[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk1(1:length(Ftrunk1)-1)));
fprintf(fd,"%d};\n",Ftrunk1(end));

fprintf(fd,"short filt96_32_2[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk2(1:length(Ftrunk2)-1)));
fprintf(fd,"%d};\n",Ftrunk2(end));

fprintf(fd,"short filt96_32_3[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk3(1:length(Ftrunk3)-1)));
fprintf(fd,"%d};\n",Ftrunk3(end));

fprintf(fd,"short filt96_32_4[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk4(1:length(Ftrunk3)-1)));
fprintf(fd,"%d};\n",Ftrunk4(end));

fprintf(fd,"short filt96_32_5[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk5(1:length(Ftrunk3)-1)));
fprintf(fd,"%d};\n",Ftrunk5(end));

fprintf(fd,"short filt96_32_6[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk6(1:length(Ftrunk3)-1)));
fprintf(fd,"%d};\n",Ftrunk6(end));

fprintf(fd,"short filt96_32_7[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk7(1:length(Ftrunk3)-1)));
fprintf(fd,"%d};\n",Ftrunk7(end));

fprintf(fd,"short filt96_32_8[96] = {\n");
fprintf(fd,"%d,",real(Ftrunk8(1:length(Ftrunk3)-1)));
fprintf(fd,"%d};\n",Ftrunk8(end));

fclose(fd);
