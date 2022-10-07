clc; clear all;
dir            = input('Enter the directory path to T tracer dumps: ');
fft_size       = input('Enter the OFDM FFT size used for file parsing: ');
num_resources  = input('Enter number of PRS respurces: ');
num_gnb        = input('Enter number of active gNBs: ');
num_prs_symb   = 1;
start_resource = 0;
buff_offset    = start_resource*2*fft_size;

%% Channel Impulse Response(CIR)
figure, hold on;
for i=0:num_gnb-1
  for j=0:num_resources-1
    file = [dir '/chT_gnb', num2str(i), '_', num2str(j), '.raw'];
    fid = fopen(file, 'r');
    if (fid > 0)
        x = fread(fid, Inf, 'int16');
    else
        disp(['Failed to open the file ', file, '..!!'])
        return;
    end
    fclose(fid);
    
    y = x(buff_offset+1:2:num_prs_symb*2*fft_size) + 1j*x(buff_offset+2:2:num_prs_symb*2*fft_size);
    plot(abs(fftshift(y)));
  end
end
xlabel('FFT Index'); ylabel('ABS');
title('CHANNEL IMPULSE RESPONSE');
hold off;

%% Channel Frequncy Response(CFR)
figure, hold on;
for i=0:num_gnb-1
  for j=0:num_resources-1
    file = [dir '/chF_gnb', num2str(i), '_', num2str(j), '.raw'];
    fid = fopen(file, 'r');
    if (fid > 0)
        x = fread(fid, Inf, 'int16');
    else
        disp(['Failed to open the file ', file, '..!!'])
        return;
    end
    fclose(fid);
    
    y = x(buff_offset+1:2:num_prs_symb*2*fft_size) + 1j*x(buff_offset+2:2:num_prs_symb*2*fft_size);
    plot(abs(y));
  end
end
xlabel('FFT Index'); ylabel('ABS');
title('CHANNEL FREQUENCY RESPONSE');
hold off;