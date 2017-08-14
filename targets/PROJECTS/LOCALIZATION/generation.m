addpath('../../../openair1/SIMULATION/LTE_PHY/')
addpath('../../../openair1/PHY/LTE_ESTIMATION/')
addpath('../../../openair1/PHY/LTE_REFSIG/')
addpath('../../../targets/ARCH/EXMIMO/USERSPACE/OCTAVE')

nb_rb = 100;
num_carriers = 2048/100*nb_rb;
num_zeros = num_carriers-(12*nb_rb+1);
prefix_length = num_carriers/4;
num_symbols_frame = 120;
preamble_length = 120;

[s,f] = OFDM_TX_FRAME(num_carriers,num_zeros,prefix_length,num_symbols_frame,preamble_length);
s=s*sqrt(num_carriers);

primary_synch;
primary_synch0_time;

pss0_up = interp(primary_synch0_time,num_carriers/128);
pss0_up_cp = [pss0_up(num_carriers-prefix_length+1:end) pss0_up];

s(1:num_carriers+prefix_length) = pss0_up_cp;

plot(abs(s))

save(sprintf('ofdm_pilots_sync_%d.mat',num_carriers),'-v7','s','f','num_carriers','num_zeros','prefix_length','num_symbols_frame','preamble_length');
mat2wv(s, sprintf('ofdm_pilots_sync_%d.wv',num_carriers), 30.72e6/2048*num_carriers, 1)
