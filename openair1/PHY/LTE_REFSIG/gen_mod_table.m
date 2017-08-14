% octave srcipt to generate table of modulated symbols for LEON3
% author: florian.kaltenberger@eurcom.fr, raymond.knopp@eurecom.fr
% date: 10.11.2009
%
% 0        .. "0"
% 1  - 4   .. QPSK(0)  - QPSK(3)
% 5  - 20  .. QAM16(0) - QAM16(15)
% 21 - 84  .. QAM64(0) - QAM64(63)
% 85 - 147 .. ZC(0)    - ZC(62)
% 148 - 151 .. 2^14 - 2^11

% 2^15 /sqrt(2) K = 768;
K = 768*sqrt(2)/2^15;

% Amplitude for QPSK (\f$ 2^15 \times 1/\sqrt{2}\f$)
QPSK = 23170;

% First Amplitude for QAM16 (\f$ 2^15 \times 2/\sqrt{10}\f$)
QAM16_n1 = 20724;
% Second Amplitude for QAM16 (\f$ 2^15 \times 1/\sqrt{10}\f$)
QAM16_n2 = 10362;

% First Amplitude for QAM64 (\f$ 2^14 \times 4/\sqrt{42}\f$)
QAM64_n1 = 20225;
% Second Amplitude for QAM64 (\f$ 2^14 \times 2/\sqrt{42}\f$)
QAM64_n2 = 10112;
% Third Amplitude for QAM64 (\f$ 2^14 \times 1/\sqrt{42}\f$)
QAM64_n3 = 5056;

%% QPSK
for r=0:1 %0 -- 1 LS
for j=0:1 %0 -- 1 MS
qpsk_table2(2*r+j+1) = ((1-r*2)*QPSK +  1j*(1-2*j)*QPSK);
end
end



%% QAM16
  for a=-1:2:1 
    for b=-1:2:1
	index = (1+a) + (1+b)/2;  
	qam16_table(index+1) = -a*(QAM16_n1 + (b*QAM16_n2)); 
    end
  end

for b0=0:1
for b1=0:1
for b2=0:1
for b3=0:1
qam16_table2(b0*8+b1*4+b2*2+b3*1+1) = qam16_table(b0*2+b2*1+1) + 1j*qam16_table(b1*2+b3*1+1);
end
end
end
end

%% QAM64
  for a=-1:2:1 
    for b=-1:2:1 
      for c=-1:2:1
	index = (1+a)*2 + (1+b) + (1+c)/2;  
	qam64_table(index+1) = -a*(QAM64_n1 + b*(QAM64_n2 + (c*QAM64_n3)));
      end
    end
  end	

for b0=0:1
for b1=0:1
for b2=0:1
for b3=0:1
for b4=0:1
for b5=0:1
qam64_table2(b0*32+b1*16+b2*8+b3*4+b4*2+b5*1+1) = qam64_table(b0*4+b2*2+b4*1+1) + 1j*qam64_table(b1*4+b3*2+b5*1+1);
end
end
end
end
end
end



%% PSS
psync_table = floor(32767*exp(-1j*2*pi*(0:62)/63));

%% Test signals 
test_table = pow2(14:-1:11).';

table = round(K * [0; qpsk_table2(:); qam16_table2(:); qam64_table2(:); psync_table(:)]); 
table = [table; test_table];

save mod_table.mat table

table2 = zeros(1,length(table)*2);
table2(1:2:end) = real(table);
table2(2:2:end) = imag(table);

fd = fopen("mod_table.h","w");
fprintf(fd,"#define MOD_TABLE_SIZE_SHORT %d\n", length(table)*2);
fprintf(fd,"#define MOD_TABLE_QPSK_OFFSET %d\n", 1);
fprintf(fd,"#define MOD_TABLE_16QAM_OFFSET %d\n",5);
fprintf(fd,"#define MOD_TABLE_64QAM_OFFSET %d\n",21);
fprintf(fd,"#define MOD_TABLE_PSS_OFFSET %d\n",85);
fprintf(fd,"short mod_table[MOD_TABLE_SIZE_SHORT] = {");
fprintf(fd,"%d,",table2(1:end-1));
fprintf(fd,"%d};\n",table2(end));
fclose(fd);
