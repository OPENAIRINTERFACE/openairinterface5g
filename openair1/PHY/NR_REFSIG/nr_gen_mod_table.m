% octave script for generating NR specific modulated symbols
%
% 0        .. "0"
% 1,2      .. BPSK(0),BPSK(1)

% Amplitude for BPSK (\f$ 2^15 \times 1/\sqrt{2}\f$)
BPSK = 23170;

% Amplitude for QPSK (\f$ 2^15 \times 1/\sqrt{2}\f$)
QPSK = 23170;

% BPSK
for b = 0:1
bpsk_table(b+1) = (1 - 2*b)*BPSK + 1j*(1-2*b)*BPSK;
end

%% QPSK
for r=0:1 %0 -- 1 LS
for j=0:1 %0 -- 1 MS
qpsk_table(2*r+j+1) = ((1-r*2)*QPSK +  1j*(1-2*j)*QPSK);
end
end

table = [ 0; bpsk_table(:); qpsk_table(:) ];

save mod_table.mat table

table2 = zeros(1,length(table)*2);
table2(1:2:end) = real(table);
table2(2:2:end) = imag(table);

fd = fopen("nr_mod_table.h","w");
fprintf(fd,"#define NR_MOD_TABLE_SIZE_SHORT %d\n", length(table)*2);
fprintf(fd,"#define NR_MOD_TABLE_BPSK_OFFSET %d\n", 1);
fprintf(fd,"#define NR_MOD_TABLE_QPSK_OFFSET %d\n", 3);
fprintf(fd,"short nr_mod_table[MOD_TABLE_SIZE_SHORT] = {");
fprintf(fd,"%d,",table2(1:end-1));
fprintf(fd,"%d};\n",table2(end));
fclose(fd);
