alpha = [-0.4616 + 0.5439i, 0.2806 + 0.6476i, -0.1136 - 0.6818i, 0.6944 + 0.5043i, 0.4072 + 0.5626i, -0.7753 + 0.1776i];

beta = [ 0.0225 - 0.0595i, 0.0088 + 0.0602i, 0.0307 + 0.0555i, -0.0244 - 0.0028i, 0.0828 - 0.2378i, 0.4194 - 0.2429i];

n_tx = 2;
n_rx = 1;

A = zeros(length(alpha),n_tx,n_tx);
B = zeros(length(alpha),n_rx,n_rx);
C = zeros(length(alpha),n_tx*n_rx, n_tx*n_rx);
D = zeros(length(alpha),n_tx*n_rx, n_tx*n_rx);

fprintf(1,'double R%d%d_sqrt[%d][%d] = {\n', n_tx, n_rx, length(alpha), 2*(n_tx*n_rx)^2);

for i=1:length(alpha)
    if (n_tx==1)
        A(i,:,:) = 1;
    elseif (n_tx==2)
        A(i,:,:) = [1 conj(alpha(i)); alpha(i) 1];
    else
        error('n_tx must be <= 2')
    end
    
    if (n_rx==1)
        B(i,:,:) = 1;
    elseif (n_rx==2)
        B(i,:,:) = [1 conj(beta(i)); beta(i) 1];
    else
        error('n_tx must be <= 2')
    end
    
    C(i,:,:) = kron(squeeze(A(i,:,:)),squeeze(B(i,:,:)));
    
    D(i,:,:) = sqrtm(squeeze(C(i,:,:)));

    fprintf(1,'{');
    for j=1:((n_tx*n_rx)^2-1)
        fprintf(1,'%f, %f,', real(D(i,j)), imag(D(i,j))); 
    end
    j=(n_tx*n_rx)^2;
    fprintf(1,'%f, %f},\n', real(D(i,j)), imag(D(i,j))); 

end
fprintf(1,'};\n');    

