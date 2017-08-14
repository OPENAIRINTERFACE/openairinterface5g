% this script generates the .h file for the correlation matrices of the
% SCM-C for the 4x4 case (with polarization) as well as the 2x2 case
% (without polarization)

alpha_beta = ...
    [-0.4616 + 0.5439i  0.0225 - 0.0595i;
    0.2806 + 0.6476i  0.0088 + 0.0602i;
    -0.1136 - 0.6818i  0.0307 + 0.0555i;
    0.6944 + 0.5043i -0.0244 - 0.0028i;
    0.4072 + 0.5626i  0.0828 - 0.2378i;
    -0.7753 + 0.1776i  0.4194 - 0.2429i];

alpha = alpha_beta(:,1); 
beta  = alpha_beta(:,2); 

Gamma = ...
    [0.5953 0.4047 0 0;
    0.4047 0.5953 0 0;
    0 0 0.5953 -0.4047;
    0 0 -0.4047 0.5953;
    0.6134 0.3866 0 0;
    0.3866 0.6134 0 0;
    0 0 0.6134 -0.3866;
    0 0 -0.3866 0.6134;
    0.6090 0.3910 0 0;
    0.3910 0.6090 0 0;
    0 0 0.6090 -0.3910;
    0 0 -0.3910 0.6090;
    0.6430 0.3570 0 0;
    0.3570 0.6430 0 0;
    0 0 0.6430 -0.3570;
    0 0 -0.3570 0.6430;
    0.6935 0.3065 0 0;
    0.3065 0.6935 0 0;
    0 0 0.6935 -0.3065;
    0 0 -0.3065 0.6935;
    0.7535  0.2465 0 0;
    0.2465 0.7535 0 0;
    0  0  0.7535 -0.2465;
    0 0 -0.2465 0.7535];

Gamma = reshape(Gamma.',4,4,[]);

%%
for i=1:size(Gamma,3)
    A = [1 conj(alpha(i)); alpha(i) 1];
    B = [1 conj(beta(i)); beta(i) 1];
    R44(:,:,i) = kron(kron(A,Gamma(:,:,i)),B);
    R22(:,:,i) = kron(A,B);
    R22_sqrt(:,:,i) = sqrtm(R22(:,:,i));
end

%%
R22_sqrt_int = zeros(8,4,6);
R22_sqrt_int(1:2:end,:,:) = real(R22_sqrt);
R22_sqrt_int(2:2:end,:,:) = imag(R22_sqrt);

%%
fid = fopen('scm_corrmat.h','w');
fprintf(fid,'double R22_sqrt[][] = {\n');
for i=1:size(Gamma,3)
    fprintf(fid,'{');
    fprintf(fid,'%f, ',R22_sqrt_int(:,:,i)); 
    fprintf(fid,'\b\b},\n');
end
fprintf(fid,'};\n');
fclose(fid)