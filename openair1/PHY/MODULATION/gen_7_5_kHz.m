
function [] = gen_7_5_kHz()
[s6_n2, s6_e2] = gen_sig(6);
[s15_n2, s15_e2] = gen_sig(15);
[s25_n2, s25_e2] = gen_sig(25);
[s50_n2, s50_e2] = gen_sig(50);
[s75_n2, s75_e2] = gen_sig(75);
[s100_n2, s100_e2] = gen_sig(100);


fd=fopen("kHz_7_5.h","w");
fprintf(fd,"s16 s6n_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s6_n2));
fprintf(fd,"%d,",s6_n2(1:(end-1)));
fprintf(fd,"%d};\n\n",s6_n2(end));

fprintf(fd,"s16 s6e_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s6_e2));
fprintf(fd,"%d,",s6_e2(1:(end-1)));
fprintf(fd,"%d};\n\n",s6_e2(end));

fprintf(fd,"s16 s15n_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s15_n2));
fprintf(fd,"%d,",s15_n2(1:(end-1)));
fprintf(fd,"%d};\n\n",s15_n2(end));

fprintf(fd,"s16 s15e_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s15_e2));
fprintf(fd,"%d,",s15_e2(1:(end-1)));
fprintf(fd,"%d};\n\n",s15_e2(end));

fprintf(fd,"s16 s25n_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s25_n2));
fprintf(fd,"%d,",s25_n2(1:(end-1)));
fprintf(fd,"%d};\n\n",s25_n2(end));

fprintf(fd,"s16 s25e_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s25_e2));
fprintf(fd,"%d,",s25_e2(1:(end-1)));
fprintf(fd,"%d};\n\n",s25_e2(end));

fprintf(fd,"s16 s50n_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s50_n2));
fprintf(fd,"%d,",s50_n2(1:(end-1)));
fprintf(fd,"%d};\n\n",s50_n2(end));

fprintf(fd,"s16 s50e_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s50_e2));
fprintf(fd,"%d,",s50_e2(1:(end-1)));
fprintf(fd,"%d};\n\n",s50_e2(end));

fprintf(fd,"s16 s75n_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s75_n2));
fprintf(fd,"%d,",s75_n2(1:(end-1)));
fprintf(fd,"%d};\n\n",s75_n2(end));

fprintf(fd,"s16 s75e_kHz_7_5[%d]__attribute__((aligned(16))) = {",length(s75_e2));
fprintf(fd,"%d,",s75_e2(1:(end-1)));
fprintf(fd,"%d};\n\n",s75_e2(end));

fprintf(fd,"s16 s100n_kHz_7_5[%d]__attribute__((aligned(16)))= {",length(s100_n2));
fprintf(fd,"%d,",s100_n2(1:(end-1)));
fprintf(fd,"%d};\n\n",s100_n2(end));

fprintf(fd,"s16 s100e_kHz_7_5[%d]__attribute__((aligned(16)))= {",length(s100_n2));
fprintf(fd,"%d,",s100_e2(1:(end-1)));
fprintf(fd,"%d};\n\n",s100_e2(end));

fclose(fd);
end


function [s_n2, s_e2] = gen_sig(RB)
    % 20MHz BW
    cp0 = 160;
    cp = 144;
    cpe = 512;
    samplerate = 30.72e6;
    ofdm_size = 2048;
    len = 15360;
    switch(RB)
        case 6
            ratio = 1/16;
        case 15
            ratio = 1/8;
        case 25
            ratio = 1/4;
        case 50
            ratio = 1/2;
        case 75
            ratio = 3/4;
        case 100
            ratio = 1;
        otherwise
        disp("Wrong Number of RB");
    end
    cp0 = cp0*ratio;
    cp = cp*ratio;
    cpe = cpe*ratio;
    samplerate = samplerate*ratio;
    ofdm_size = ofdm_size*ratio;
    len = len*ratio;
    
    s_n0 = floor(32767*exp(-sqrt(-1)*2*pi*(-cp0:ofdm_size-1)*7.5e3/samplerate));
    s_n1 = floor(32767*exp(-sqrt(-1)*2*pi*(-cp:ofdm_size-1)*7.5e3/samplerate));
    s_n = [s_n0 s_n1 s_n1 s_n1 s_n1 s_n1 s_n1];
    s_n2 = zeros(1, 2*len);
    s_n2(1:2:end) = real(s_n);
    s_n2(2:2:end) = imag(s_n);
    s_e = floor(32767*exp(-sqrt(-1)*2*pi*(-cpe:ofdm_size-1)*7.5e3/samplerate));
    s_e = [s_e s_e s_e s_e s_e s_e];
    s_e2 = zeros(1, 2*len);
    s_e2(1:2:end) = real(s_e);
    s_e2(2:2:end) = imag(s_e);
end

