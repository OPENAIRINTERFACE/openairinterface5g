if ~exist('UE_txs0','var')
  UE_txsig0;
end
if (~exist('UE_txs1','var') && exist('UE_txsig1','file'))
  UE_txsig1;
end
if ~exist('UE_rxs0','var')
  UE_rxsig0;
end
if (~exist('UE_rxs1','var') && exist('UE_rxsig1','file'))
  UE_rxsig1;
end

if ~exist('UE_txs0_1', 'var')
  UE_txsig0_1;
end
if ~exist('UE_txs1_1','var')
  UE_txsig1_1;
end
if ~exist('UE_rxs0_1','var')
  UE_rxsig0_1;
end
if ~exist('UE_rxs1_1','var')
  UE_rxsig1_1;
end
if ~exist('UE_rxs0_2','var')
  UE_rxsig0_2;
end
if (~exist('UE_rxs1_2','var') && exist('UE_rxsig1_2','file'))
  UE_rxsig1_2;
end

if ~exist('eNb_txs0','var')
  eNb_txsig0;
end
if (~exist('eNb_txs1','var') && exist('eNb_txsig1','file'))
  eNb_txsig1;
end
if ~exist('eNb_rxs0','var')
  eNb_rxsig0;
end
if (~exist('eNb_rxs1','var') && exist('eNb_rxsig1','file'))
  eNb_rxsig1;
end

if ~exist('eNb_txs0_1', 'var')
  eNb_txsig0_1;
end
if ~exist('eNb_txs1_1','var')
  eNb_txsig1_1;
end
if ~exist('eNb_rxs0_1','var')
  eNb_rxsig0_1;
end
if ~exist('eNb_rxs1_1','var')
  eNb_rxsig1_1;
end
if (~exist('eNb_rxs0_2','var') && exist('eNb_rxsig0_2','file'))
  eNb_rxsig0_2;
end
if (~exist('eNb_rxs1_2','var') && exist('eNb_rxsig1_2','file'))
  eNb_rxsig1_2;
end

samples_per_symbol = 640;
symbols_per_slot = 6;
slots_per_frame = 20;
x_slot = [0:1/(samples_per_symbol*symbols_per_slot):((slots_per_frame-0)-(1/(samples_per_symbol*symbols_per_slot)))];

figure(1)
subplot(2,2,1)
plot(x_slot,abs(eNb_txs0));
title('Primary eNb tx - antenna 0 - time');
if exist('eNb_txs1','var')
  subplot(2,2,2)
  plot(x_slot,abs(eNb_txs1));
  title('Primary eNb tx - antenna 1 - time');
end
subplot(2,2,3)
plot(x_slot,abs(UE_rxs0));
title('Primary UE rx - antenna 0 - time');
subplot(2,2,4)
plot(x_slot,abs(UE_rxs0_2));
title('Received interference at Primary UE');

print('prim_DL.pstex','-dpstex');

figure(2)
subplot(2,2,1)
plot(x_slot,abs(eNb_txs0_1));
title('Secondary eNb tx - antenna 0 - time');
subplot(2,2,2)
plot(x_slot,abs(eNb_txs1_1));
title('Secondary eNb tx - antenna 1 - time');
subplot(2,2,3)
plot(x_slot,abs(UE_rxs0_1));
title('Secondary UE rx - antenna 0 - time');
subplot(2,2,4)
plot(x_slot,abs(UE_rxs1_1));
title('Secondary UE rx - antenna 1 - time');

print('seco_DL.pstex','-dpstex');

figure(3)
subplot(2,2,1)
plot(x_slot,abs(UE_txs0));
title('Primary UE tx - antenna 0 - time');

subplot(2,2,3)
plot(x_slot,abs(eNb_rxs0));
title('Primary eNb rx - antenna 0 - time');
if exist('eNb_rxs1','var')
  subplot(2,2,4)
  plot(x_slot,abs(eNb_rxs1));
  title('Primary eNb rx - antenna 1 - time');
elseif exist('eNb_rxs0_2','var')
  subplot(2,2,4)
  plot(x_slot,abs(eNb_rxs0_2));
  title('Received interference at Primary eNb');
end

print('prim_UL.pstex','-dpstex');

figure(4)
subplot(2,2,1)
plot(x_slot,abs(UE_txs0_1));
title('Secondary UE tx - antenna 0 - time');
subplot(2,2,2)
plot(x_slot,abs(UE_txs1_1));
title('Secondary UE tx - antenna 1 - time');
subplot(2,2,3)
plot(x_slot,abs(eNb_rxs0_1));
title('Secondary eNb rx - antenna 0 - time');
subplot(2,2,4)
plot(x_slot,abs(eNb_rxs1_1));
title('Secondary eNb rx - antenna 1 - time');

print('seco_UL.pstex','-dpstex');