% OCTAVE Code to generate upsampled pss0-2

n=0:61;
d0 = zeros(1,62);
d1 = zeros(1,62);
d2 = zeros(1,62);
d0(1+(0:30)) = exp(-sqrt(-1)*pi*25*(0:30).*(1:31)/63);
d1(1+(0:30)) = exp(-sqrt(-1)*pi*29*(0:30).*(1:31)/63);
d2(1+(0:30)) = exp(-sqrt(-1)*pi*34*(0:30).*(1:31)/63);
d0(1+(31:61)) = exp(-sqrt(-1)*pi*25*(32:62).*(33:63)/63);
d1(1+(31:61)) = exp(-sqrt(-1)*pi*29*(32:62).*(33:63)/63);
d2(1+(31:61)) = exp(-sqrt(-1)*pi*34*(32:62).*(33:63)/63);

pss0f = zeros(1,2048);
pss0f(2:32) = d0(32:62);
pss0f(2048+(-30:0)) = d0(1:31);

pss1f = zeros(1,2048);
pss1f(2:32) = d1(32:62);
pss1f(2048+(-30:0)) = d1(1:31);

pss2f = zeros(1,2048);
pss2f(2:32) = d2(32:62);
pss2f(2048+(-30:0)) = d2(1:31);

pss0_6144f = fft(ifft(pss0f)*sqrt(2048),6144)/sqrt(6144);
pss1_6144f = fft(ifft(pss1f)*sqrt(2048),6144)/sqrt(6144);
pss2_6144f = fft(ifft(pss2f)*sqrt(2048),6144)/sqrt(6144);

pss0_6144_fp = zeros(1,512);
pss0_6144_fp(1:2:256) = (floor(32767*real(pss0_6144f(6144+(-127:0)))));
pss0_6144_fp(2:2:256) = (floor(32767*imag(pss0_6144f(6144+(-127:0)))));
pss0_6144_fp(256+(1:2:256)) = (floor(32767*real(pss0_6144f(1:128))));
pss0_6144_fp(256+(2:2:256)) = (floor(32767*imag(pss0_6144f(1:128))));

pss1_6144_fp = zeros(1,512);
pss1_6144_fp(1:2:256) = (floor(32767*real(pss1_6144f(6144+(-127:0)))));
pss1_6144_fp(2:2:256) = (floor(32767*imag(pss1_6144f(6144+(-127:0)))));
pss1_6144_fp(256+(1:2:256)) = (floor(32767*real(pss1_6144f(1:128))));
pss1_6144_fp(256+(2:2:256)) = (floor(32767*imag(pss1_6144f(1:128))));

pss2_6144_fp = zeros(1,512);
pss2_6144_fp(1:2:256) = (floor(32767*real(pss2_6144f(6144+(-127:0)))));
pss2_6144_fp(2:2:256) = (floor(32767*imag(pss2_6144f(6144+(-127:0)))));
pss2_6144_fp(256+(1:2:256)) = (floor(32767*real(pss2_6144f(1:128))));
pss2_6144_fp(256+(2:2:256)) = (floor(32767*imag(pss2_6144f(1:128))));

fprintf("static int16_t pss6144_0_0[512]__attribute__((aligned(16)))={");
fprintf("%d,",pss0_6144_fp(1:511));
fprintf("%d};\n",pss0_6144_fp(512));
fprintf("static int16_t pss6144_0_1[512]__attribute__((aligned(16)))={0,0,");
fprintf("%d,",pss0_6144_fp(1:509));
fprintf("%d};\n",pss0_6144_fp(510));
fprintf("static int16_t pss6144_0_2[512]__attribute__((aligned(16)))={0,0,0,0,");
fprintf("%d,",pss0_6144_fp(1:507));
fprintf("%d};\n",pss0_6144_fp(508));
fprintf("static int16_t pss6144_0_3[512]__attribute__((aligned(16)))={0,0,0,0,0,0,");
fprintf("%d,",pss0_6144_fp(1:505));
fprintf("%d};\n",pss0_6144_fp(506));

fprintf("static int16_t pss6144_1_0[512]__attribute__((aligned(16)))={");
fprintf("%d,",pss1_6144_fp(1:511));
fprintf("%d};\n",pss1_6144_fp(512));
fprintf("static int16_t pss6144_1_1[512]__attribute__((aligned(16)))={0,0");
fprintf("%d,",pss1_6144_fp(1:509));
fprintf("%d};\n",pss0_6144_fp(510));
fprintf("static int16_t pss6144_1_2[512]__attribute__((aligned(16)))={0,0,0,0,");
fprintf("%d,",pss1_6144_fp(1:507));
fprintf("%d};\n",pss1_6144_fp(508));
fprintf("static int16_t pss6144_1_3[512]__attribute__((aligned(16)))={0,0,0,0,0,0,");
fprintf("%d,",pss1_6144_fp(1:505));
fprintf("%d};\n",pss1_6144_fp(506));

fprintf("static int16_t pss6144_2_0[512]__attribute__((aligned(16)))={");
fprintf("%d,",pss2_6144_fp(1:511));
fprintf("%d};\n",pss2_6144_fp(512));
fprintf("static int16_t pss6144_2_1[512]__attribute__((aligned(16)))={0,0,");
fprintf("%d,",pss2_6144_fp(1:509));
fprintf("%d};\n",pss2_6144_fp(510));
fprintf("static int16_t pss6144_2_2[512]__attribute__((aligned(16)))={0,0,0,0,");
fprintf("%d,",pss2_6144_fp(1:507));
fprintf("%d};\n",pss2_6144_fp(508));
fprintf("static int16_t pss6144_2_3[512]__attribute__((aligned(16)))={0,0,0,0,0,0,");
fprintf("%d,",pss2_6144_fp(1:505));
fprintf("%d};\n",pss2_6144_fp(506));
