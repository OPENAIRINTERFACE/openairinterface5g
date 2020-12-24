% octave script for generating NR specific modulated symbols
%
% 0        .. "0"
% 1,2      .. BPSK(0),BPSK(1)

K = 1/sqrt(2);

% Amplitude for BPSK (\f$ 2^15 \times 1/\sqrt{2}\f$)
BPSK = 23170;

% Amplitude for QPSK (\f$ 2^15 \times 1/\sqrt{2}\f$)
QPSK = 23170;


% First Amplitude for QAM16 (\f$ 2^15 \times 2/\sqrt{10}\f$)
QAM16_n1 = 20724;
% Second Amplitude for QAM16 (\f$ 2^15 \times 1/\sqrt{10}\f$)
QAM16_n2 = 10362;

% First Amplitude for QAM64 (\f$ 2^15 \times 4/\sqrt{42}\f$)
QAM64_n1 = 20225;
% Second Amplitude for QAM64 (\f$ 2^15 \times 2/\sqrt{42}\f$)
QAM64_n2 = 10112;
% Third Amplitude for QAM64 (\f$ 2^15 \times 1/\sqrt{42}\f$)
QAM64_n3 = 5056;


% First Amplitude for QAM256 (\f$ 2^15 \times 8/\sqrt{170}\f$)
QAM256_n1 = 20105;
% Second Amplitude for QAM256 (\f$ 2^15 \times 4/\sqrt{170}\f$)
QAM256_n2 = 10052;
% Third Amplitude for QAM256 (\f$ 2^15 \times 2/\sqrt{170}\f$)
QAM256_n3 = 5026;
% Third Amplitude for QAM256 (\f$ 2^15 \times 1/\sqrt{170}\f$)
QAM256_n4 = 2513;


% BPSK
for b = 0:1
bpsk_table(b+1) = (1 - 2*b)*BPSK + 1j*(1-2*b)*BPSK;
end

%% QPSK
for r=0:1 %0 -- 1 LS
for j=0:1 %0 -- 1 MS
  %% Formula is dispalayed

qpsk_table(2*j+r+1) = ((1-r*2)*QPSK +  1j*(1-2*j)*QPSK);
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
qam16_table2(b3*8+b2*4+b1*2+b0*1+1) = qam16_table(b0*2+b2*1+1) + 1j*qam16_table(b1*2+b3*1+1);
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
qam64_table2(b5*32+b4*16+b3*8+b2*4+b1*2+b0*1+1) = qam64_table(b0*4+b2*2+b4*1+1) + 1j*qam64_table(b1*4+b3*2+b5*1+1);
end
end
end
end
end
end


%%256QAM
%% QAM256 *******************************************************************************************
  for a=-1:2:1 
    for b=-1:2:1 
      for c=-1:2:1
        for d = -1:2:1
   
	index = (1+a)*4 + (1+b)*2 + (1+c)+(1+d)/2;  
	qam256_table(index+1) = -a*(QAM256_n1 + b*(QAM256_n2 + (c*(QAM256_n3+d*QAM256_n4))));
      end
    end
  end	
end	
for b0=0:1
for b1=0:1
for b2=0:1
for b3=0:1
for b4=0:1
for b5=0:1
for b6=0:1
for b7=0:1

%%qam64_table2(b0*32+b1*16+b2*8+b3*4+b4*2+b5*1+1) = qam64_table(b0*4+b2*2+b4*1+1) + 1j*qam64_table(b1*4+b3*2+b5*1+1);
qam256_table2(b7*128+b6*64+b5*32+b4*16+b3*8+b2*4+b1*2+b0*1+1) = qam256_table(b0*8+b2*4+b4*2+b6*1+1) + 1j*qam256_table(b1*8+b3*4+b5*2+b7*1+1);
end
end
end
end
end
end
end
end

%%QPSK byte
for b0=0:1
for b1=0:1
for b2=0:1
for b3=0:1
for b4=0:1
for b5=0:1
for b6=0:1
for b7=0:1
qpsk_byte_table2(b7*128+b6*64+b5*32+b4*16+b3*8+b2*4+b1*2+b0*1+1,:)=[(1-b0*2)*QPSK (1-2*b1)*QPSK (1-b2*2)*QPSK (1-2*b3)*QPSK ...
    (1-b4*2)*QPSK (1-2*b5)*QPSK (1-b6*2)*QPSK (1-2*b7)*QPSK];

end
end
end
end
end
end
end
end

%%16QAM byte
qam16_byte_table2=[];
for b0=0:1
for b1=0:1
for b2=0:1
for b3=0:1
for b4=0:1
for b5=0:1
for b6=0:1
for b7=0:1
qam16_byte_table2(b7*128+b6*64+b5*32+b4*16+b3*8+b2*4+b1*2+b0*1+1,:)=[qam16_table(b0*2+b2*1+1) qam16_table(b1*2+b3*1+1) ...
    qam16_table(b4*2+b6*1+1) qam16_table(b5*2+b7*1+1) ];

end
end
end
end
end
end
end
end

table = round(K * [ 0; bpsk_table(:); qpsk_table(:);qam16_table2(:); qam64_table2(:);qam256_table2(:) ]);
%scatter (real(qam256_table2), imag(qam256_table2), 'x');
save mod_table.mat table

table2 = zeros(1,length(table)*2);
table2(1:2:end) = real(table);
table2(2:2:end) = imag(table);
qpsk_byte_table2=round(K * qpsk_byte_table2'(:));
qam16_byte_table2=round(K * qam16_byte_table2'(:));

fd = fopen("nr_mod_table.h","w");
fprintf(fd,"#define NR_MOD_TABLE_SIZE_SHORT %d\n", length(table)*2);
fprintf(fd,"#define NR_MOD_TABLE_BPSK_OFFSET %d\n", 1);
fprintf(fd,"#define NR_MOD_TABLE_QPSK_OFFSET %d\n", 3);
fprintf(fd,"#define NR_MOD_TABLE_QAM16_OFFSET %d\n", 7);
fprintf(fd,"#define NR_MOD_TABLE_QAM64_OFFSET %d\n", 23);
fprintf(fd,"#define NR_MOD_TABLE_QAM256_OFFSET %d\n", 87);
fprintf(fd,"short nr_mod_table[NR_MOD_TABLE_SIZE_SHORT] = {");
fprintf(fd,"%d,",table2(1:end-1));
fprintf(fd,"%d};\n",table2(end));
fprintf(fd,"short nr_qpsk_mod_table[2048] = {");
fprintf(fd,"%d,",qpsk_byte_table2(1:end-1));
fprintf(fd,"%d};\n",qpsk_byte_table2(end));
fprintf(fd,"short nr_qam16_mod_table[1024] = {");
fprintf(fd,"%d,",qam16_byte_table2(1:end-1));
fprintf(fd,"%d};\n",qam16_byte_table2(end));
fclose(fd);
