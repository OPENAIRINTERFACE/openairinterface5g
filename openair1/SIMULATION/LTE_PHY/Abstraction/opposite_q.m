function q = opposite_q(qq)

switch qq
    case 0
        q=1;
    case 1
        q=0;
    case 2
        q=3;
    case 3
        q=2;
        
    otherwise
        disp('input must be between 0-3');
end

