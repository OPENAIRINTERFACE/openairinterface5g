%
% PURPOSE :  TLS solution for AX = B based on alternative projection
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
% Mai-05-2014  X. JIANG       0.1     creation of code
%
% REFERENCES/NOTES/COMMENTS :
%
% - none.
%
%**********************************************************************************************

function [X_est A_est B_est] = f_tls_ap(A,B)
    
%** initlisation **
    e_new = 0;
    e_old = 1e14;
    e_thr = 1e-5;            %error threshold: what if the error cannot fall down under the error threshold
    X_est = eye(size(A,2));
    A_est = A;
    
%** alternative projection **
while(abs(e_new-e_old)>e_thr)
    e_old = e_new;
    
    % optimise X_est
    X_est = (A_est'*A_est)\A_est'*B;
    
    %optimise A_est
    A_est = B*X_est'/(X_est*X_est');
    
    e_new = norm(A_est*X_est-B)^2+norm(A_est-A)^2;
end
    B_est = A_est*X_est;

end
