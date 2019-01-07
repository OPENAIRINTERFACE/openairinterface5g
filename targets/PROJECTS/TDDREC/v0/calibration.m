initparams;
N_loc = 3; %check
CHA2B = {}; %this should be a cell array
CHB2A = {}; %this should be a cell array

% run measurements for all location
for loc = 1:N_loc
    runmeas_woduplex;
    % alternatively load the measurements from file for testing
    % now you should have chanestA2B, fchanestA2B, tchanestA2B, chanestB2A,
    % fchanestB2A, tchanestB2A
    CHA2B{loc} = chanestsA2B; %to check
    CHB2A{loc} = chanestsB2A; %to check
    disp('Please move the antenna to another location and press key when finished')
    pause
end

%% calculate full F matrix
Fs = zeros(Nantb,Nantb,301);
for s=1:301
  [F, HA]=alterproj(CHB2A,CHA2B, s, Nantb, N_loc, Nmeas); 
  Fs(:,:,s)=F;
  %HAs{s}=HA;
end

%% calculate the diag F matrix based on last position
Fds = zeros(Nantb,Nantb,301);
for s=1:301
    CHA = squeeze(chanestsA2B(s,:,:));
    CHB = squeeze(chanestsB2A(s,:,:));
    Fds(:,:,s)=diag(diag(CHB*CHA')./diag(CHA*CHA'));
end

%% plot F
figure(1)
clf
hold on;
for s=1:size(Fs,3);
  F=Fs(:,:,s);
  plot(diag(F),'bo')
  plot(diag(F,1),'r+')
  plot(diag(F,2),'gx')
end
%axis([-1 1 -1 1])
hold off;

