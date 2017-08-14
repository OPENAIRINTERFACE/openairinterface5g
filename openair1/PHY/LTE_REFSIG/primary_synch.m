primary_synch0 = [zeros(1,5) exp(-1j*pi*25*(0:30).*(1:31)/63) exp(-1j*pi*25*(32:62).*(33:63)/63) zeros(1,5)];
primary_synch1 = [zeros(1,5) exp(-1j*pi*29*(0:30).*(1:31)/63) exp(-1j*pi*29*(32:62).*(33:63)/63) zeros(1,5)];
primary_synch2 = [zeros(1,5) exp(-1j*pi*34*(0:30).*(1:31)/63) exp(-1j*pi*34*(32:62).*(33:63)/63) zeros(1,5)];

%psync_table = [0 exp(-j*2*pi*(0:62)/63)];
%psync_table_mod = zeros(1,63*2);
%psync_table_mod(1:2:end) = floor(32767/sqrt(2)*real(psync_table));
%psync_table_mod(2:2:end) = floor(32767/sqrt(2)*imag(psync_table));

primary_synch0_tab = [zeros(1,5) mod(25*((0:30).*(1:31))/2,63)+85 mod(25*((32:62).*(33:63))/2,63)+85 zeros(1,5)];
primary_synch1_tab = [zeros(1,5) mod(29*((0:30).*(1:31))/2,63)+85 mod(29*((32:62).*(33:63))/2,63)+85 zeros(1,5)];
primary_synch2_tab = [zeros(1,5) mod(34*((0:30).*(1:31))/2,63)+85 mod(34*((32:62).*(33:63))/2,63)+85 zeros(1,5)];

primary_synch0_mod = zeros(1,72*2);
primary_synch0_mod(1:2:end) = floor(32767*real(primary_synch0));
primary_synch0_mod(2:2:end) = floor(32767*imag(primary_synch0));

primary_synch1_mod = zeros(1,72*2);
primary_synch1_mod(1:2:end) = floor(32767*real(primary_synch1));
primary_synch1_mod(2:2:end) = floor(32767*imag(primary_synch1));

primary_synch2_mod = zeros(1,72*2);
primary_synch2_mod(1:2:end) = floor(32767*real(primary_synch2));
primary_synch2_mod(2:2:end) = floor(32767*imag(primary_synch2));

primary_synch0_mod2 = zeros(1,128);
primary_synch0_mod2((128-35):128)=primary_synch0(1:36);
primary_synch0_mod2(2:37)=primary_synch0(37:end);
primary_synch0_time = ifft(primary_synch0_mod2)*sqrt(128);

primary_synch1_mod2 = zeros(1,128);
primary_synch1_mod2((128-35):128)=primary_synch1(1:36);
primary_synch1_mod2(2:37)=primary_synch1(37:end);
primary_synch1_time = ifft(primary_synch1_mod2)*sqrt(128);

primary_synch2_mod2 = zeros(1,128);
primary_synch2_mod2((128-35):128)=primary_synch2(1:36);
primary_synch2_mod2(2:37)=primary_synch2(37:end);
primary_synch2_time = ifft(primary_synch2_mod2)*sqrt(128);

fd = fopen('primary_synch.h','w');
fprintf(fd,'short primary_synch0[144] = {');
fprintf(fd,'%d,',primary_synch0_mod(1:end-1));
fprintf(fd,'%d};\n',primary_synch0_mod(end));
fprintf(fd,'short primary_synch1[144] = {');
fprintf(fd,'%d,',primary_synch1_mod(1:end-1));
fprintf(fd,'%d};\n',primary_synch1_mod(end));
fprintf(fd,'short primary_synch2[144] = {');
fprintf(fd,'%d,',primary_synch2_mod(1:end-1));
fprintf(fd,'%d};\n',primary_synch2_mod(end));
fprintf(fd,'unsigned char primary_synch0_tab[72] = {');
fprintf(fd,'%d,',primary_synch0_tab(1:end-1));
fprintf(fd,'%d};\n',primary_synch0_tab(end));
fprintf(fd,'unsigned char primary_synch1_tab[72] = {');
fprintf(fd,'%d,',primary_synch1_tab(1:end-1));
fprintf(fd,'%d};\n',primary_synch1_tab(end));
fprintf(fd,'unsigned char primary_synch2_tab[72] = {');
fprintf(fd,'%d,',primary_synch2_tab(1:end-1));
fprintf(fd,'%d};\n',primary_synch2_tab(end));
fclose(fd);

% for LEON we need to invert the endianess
fd = fopen('primary_synch_leon.h','w');
primary_synch0_tab = reshape(primary_synch0_tab,4,[]);
primary_synch0_tab = primary_synch0_tab([4 3 2 1],:);
primary_synch0_tab = primary_synch0_tab(:);
primary_synch1_tab = reshape(primary_synch0_tab,4,[]);
primary_synch1_tab = primary_synch1_tab([4 3 2 1],:);
primary_synch1_tab = primary_synch1_tab(:);
primary_synch2_tab = reshape(primary_synch0_tab,4,[]);
primary_synch2_tab = primary_synch2_tab([4 3 2 1],:);
primary_synch2_tab = primary_synch2_tab(:);
fprintf(fd,'unsigned char primary_synch0_tab[72] = {');
fprintf(fd,'%d,',primary_synch0_tab(1:end-1));
fprintf(fd,'%d};\n',primary_synch0_tab(end));
fprintf(fd,'unsigned char primary_synch1_tab[72] = {');
fprintf(fd,'%d,',primary_synch1_tab(1:end-1));
fprintf(fd,'%d};\n',primary_synch1_tab(end));
fprintf(fd,'unsigned char primary_synch2_tab[72] = {');
fprintf(fd,'%d,',primary_synch2_tab(1:end-1));
fprintf(fd,'%d};\n',primary_synch2_tab(end));
fclose(fd);
