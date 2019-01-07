%
% PURPOSE :  TLS solution for AX = B based on SVD
%
% ARGUMENTS :
%
% A    : observation of A  
% B    : observation of B
%
% OUTPUTS :
%
% X    : TLS solution for X
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

function [X_est A_est B_est]= f_tls_svd(A,B)
    C = [A B];
    n = size(A,2);
    d = size(B,2);
    [U S V] = svd(C,0);
    V12 = V(1:n,n+1:end);
    V22 = V(n+1:end,n+1:end);
    S1 = S(1:n,1:n); 
    Z12 = zeros(n,d);
    Z22 = zeros(d);
    Z21 = zeros(d,n);
    X_est = - V12/V22;
    C_est = U*[S1 Z12;Z21 Z22]*V';
    A_est = C_est(:,1:n);
    B_est = C_est(:,n+1:end);
%     delta_C = -U*diag([zeros(n,1);diag(S(n+1:end,n+1:end))])*V';
%    delta_C = [A_est-A B_est-B]; %same as the previous calculation
%    err = norm(delta_C,'fro')^2/norm(C,'fro')^2;:w
end
