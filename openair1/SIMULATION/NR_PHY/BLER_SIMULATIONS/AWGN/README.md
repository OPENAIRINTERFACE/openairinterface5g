# Overview of AWGN BLER Table #

Look-up tables of performance versus signal-to-noise ratio (SNR) are obtained via PHY simulations in additive white Gaussian noise (AWGN) channels. Then, to evaluate the BLER of fading channel, we map the received SINR to an AWGN curve using these look-up tables. Note that due to time/frequency selective fading, the received SINR value for each packet/ resource block varies. This requires the computation of AWGN equivalent SINR, also generally known as effective SINR, by mapping the individual symbol SINRs to a scalar. Then, the performance of the PHY in a fading channel at a given effective SINR is equivalent to the performance in AWGN channel.

## AWGN_results ##

    The provided simulation results are for NR SISO channel modeling.

## AWGN_MIMO 2x2_results ##

    The provided simulation results are for NR MIMO 2x2 channel modeling.


# Table Generation Method #

The tables result from an exponential effective SINR mapping (EESM) and comparison to MATLAB link-level simulation models.

# Limitations #

These BLER curves are used in the packet dropping abstraction process.
The curves in the tables can be used under the condition of L1 abstraction mode and packet dropping abstraction process.


