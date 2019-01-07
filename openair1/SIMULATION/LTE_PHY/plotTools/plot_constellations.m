UE0_compensated;
UE0_extracted;
UE0_dl_ch_est_ext;
UE0_llr_tl0;

for i = 10:13
  figure(i)
  close(i)
end

sel = 1:1800;

UE0_comp  = reshape(UE0_comp,300,[]);
UE0_ext   = reshape(UE0_ext,300,[]);
UE0_llr   = reshape(UE0_llr,600,[]);
dl_ce_ext = reshape(dl_ce_ext(sel),300,[]);
p = 0;
for k=2:12
if (norm(UE0_ext(:,k))>0 && p<7)
  p = p + 1;
  figure(10)
  subplot(3,2,p);
  plot(UE0_comp(:,k),'x');
  title(['rxdataF_{comp}, symbol ' num2str(k-1)]);
  figure(11)
  subplot(3,2,p);
  plot(UE0_ext(:,k),'x');
  title(['rxdataF_{ext}, symbol ' num2str(k-1)]);
if (k < 7)
  figure(12)
  subplot(3,2,p);
  plot(abs(dl_ce_ext(:,k)));
  title(['dl ch estimates, symbol ' num2str(k-1)]);
else
  figure(12)
  subplot(3,2,p);
  plot(abs(dl_ce_ext(:,k-6)));
  title(['dl ch estimates, symbol ' num2str(k-1)]);
end
  figure(13)
  subplot(3,2,p);
  plot(UE0_llr(:,p),'x');
  title(['llr, symbol ' num2str(k-1)]);
end
end
