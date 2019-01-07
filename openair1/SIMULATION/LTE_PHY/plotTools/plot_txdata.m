txdata_f0;
if exist('txdata_f1.m','file');
  txdata_f1;
end
txdata_t0;
if exist('txdata_t1.m','file');
  txdata_t1;
end

figure(8)
subplot(2,2,1);
pcolor(abs(reshape(txs_f0,512,[])));
shading flat;
subplot(2,2,2);
if exist('txs_f1','var');
  pcolor(abs(reshape(txs_f1,512,[])));
  shading flat;
end
subplot(2,2,3);
plot(abs(txs_t0));
subplot(2,2,4);
if exist('txs_t1','var');
  plot(abs(txs_t1));
end
