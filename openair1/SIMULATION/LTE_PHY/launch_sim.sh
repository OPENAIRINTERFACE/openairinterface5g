#!/bin/bash
# PBS Options 
#PBS -m abe 
#PBS -d /homes/kaltenbe/Devel/openair_lte/openair1/SIMULATION/LTE_PHY

# Simulation parameters
# MCS="0 1 2 3 4 5 6 7 8 9"
# MCS="0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28"
MCS="16"
# MCS="17 18 19 20 21 22 23 24 25 26 27 28"
#CHANNEL="C E F G H I J K L M"
CHANNEL="H"
USER2="1"
NSIMUS=1000

#QPSK
#MCS="0 1 2 3 4 5 6 7 8 9"
#16QAM
#MCS="10 11 12 13 14 15 16"
#64QAM
#MCS="17 18 19 20 21 22"
#MCS="23 24 25 26 27 28"

for U in $USER2
do
    for C in $CHANNEL
    do
	case "$C" in
	    "A") ch=1;;
	    "B") ch=2;;
	    "C") ch=3;;
	    "D") ch=4;;
	    "E") ch=5;;
	    "F") ch=6;;
	    "G") ch=7;;
	    "H") ch=8;;
	    "I") ch=9;;
	    "J") ch=10;;
	    "K") ch=11;;
	    "L") ch=12;;
	    "M") ch=13;;
	esac
	touch TB_u2\=${U}_chan${ch}.tex # Create file for all mcs
	echo %$(svn info | grep Revision:) > TB_u2\=${U}_chan${ch}.tex
	echo %User2=${U},Channel=${C},MCS=[${MCS}],NFRAMES=${NSIMUS} >> TB_u2\=${U}_chan${ch}.tex
	for M in $MCS
	do
	    #./dlsim -m$M -s$(($M-10)) -x5 -y2 -z1 -g$C -f2 -u$U -n$NSIMUS -R1
	    ./dlsim -m$M -s$(($M)) -x5 -y2 -z1 -g$C -f2 -u$U -n$NSIMUS -R1
	    cat second_bler_tx5_u2\=${U}_mcs${M}_chan${ch}_nsimus${NSIMUS} >> TB_u2\=${U}_chan${ch}.tex
	    rm second_bler_tx5_u2\=${U}_mcs${M}_chan${ch}_nsimus${NSIMUS}
	done
    done
done
mkdir blerSimus
mv TB*.tex blerSimus
zip -r blerSimus.zip blerSimus
rm -r blerSimus

mkdir allCsv
mv *.csv allCsv
