##   Decodes rf_local values: [ txi, txq, rxi, rxq ] = rfldec(rflocal)

## Author: Matthias Ihmig <ihmig@solstice>
## Created: 2012-12-05

function [ txi, txq, rxi, rxq ] = rfldec(rflocal)
    txi = mod(floor( rflocal /1 ), 64)
    txq = mod(floor( rflocal /64), 64)
    rxi = mod(floor( rflocal /4096), 64)
    rxq = mod(floor( rflocal /262144), 64)
endfunction
