##   Composes rf_local values: rfl(txi, txq, rxi, rxq)

## Author: Matthias Ihmig <ihmig@solstice>
## Created: 2012-12-05

function [ ret ] = rfl(txi, txq, rxi, rxq)
    ret = txi + txq*2^6 + rxi*2^12 + rxq*2^18;
endfunction
