%
% PURPOSE :  TLS solution for AX = B based on SVD assuming X is diagonal
%
% ARGUMENTS :
%
% A    : observation of A  
% B    : observation of B
%
% OUTPUTS :
%
% X    : TLS solution for X (Diagonal)
%
%**********************************************************************************************
%                            EURECOM -  All rights reserved
%
% AUTHOR : Xiwen JIANG, Florian Kaltenberger
%
% DEVELOPMENT HISTORY :
% 
% Date         Name(s)       Version  Description
% -----------  ------------- -------  ------------------------------------------------------
% Apr-30-2014  X. JIANG       0.1     creation of code
%
% REFERENCES/NOTES/COMMENTS :
%
% - I. Markovsky and S. V. Huffel, “Overview of total least-squares methods,” Signal Processing, vol. 87, pp.
% 2283–2302, 2007  
%
%**********************************************************************************************

function [X_est A_est B_est] = f_tls_diag(A,B)

d_N = size(A,2);
X_est = zeros(d_N);
A_est = zeros(size(A));
B_est = zeros(size(B));
err_est = zeros(d_N);

for d_n = 1:d_N
    [X_est(d_n,d_n) A_est(:,d_n) B_est(:,d_n)] = f_tls_svd(A(:,d_n),B(:,d_n));
end

%method 2: LS solution
% for d_n = 1:d_N
%     X_est(d_n,d_n) = (A(:,d_n).'*A(:,d_n))\A(:,d_n).'*B(:,d_n);
% end
