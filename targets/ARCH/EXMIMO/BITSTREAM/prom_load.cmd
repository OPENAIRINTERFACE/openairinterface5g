setMode -bs
setCable -port auto
Identify -inferir
identifyMPM  
assignFile -p 1 -file "express-mimo2.bit"
attachflash -position 1 -spi "W25Q64BV"
assignfiletoattachedflash -position 1 -file "exmimo2_prom.mcs"
attachflash -position 1 -spi "W25Q64BV"
Program -p 1 -dataWidth 4 -spionly -e -v -loadfpga 
quit
