OAI Test PLAN

Obj.#   Case#   Test#	Description

01                      pre-commit test case
01      01              Build OAI 
01      01      01      Build oaisim.Rel8
01      01      02      Build oaisim.Rel10	
01      01      03      Build oaisim_noS1.Rel10	
01      01      10      Build lte-softmodem_noS1.USRP.Rel10
01      01      11      Build lte-softmodem_noS1.EXMIMO.Rel10
01      01      12      Build lte-softmodem_noS1.BLADERF.Rel10
01      01      13      Build lte-softmodem_noS1.ETHERNET.Rel10
01      01      14      Build lte-softmodem_noS1.LMSSDR.Rel10
01      01      20      Build lte-softmodem.USRP.Rel10
01      01      21      Build lte-softmodem.EXMIMO.Rel10
01      01      22      Build lte-softmodem.BLADERF.Rel10
01      01      23      Build lte-softmodem.ETHERNET.Rel10 (RCC)
01      01      24      Build lte-softmodem.LMSSDR.Rel10

01      01      30      Build (dlsim.Rel10 + ulsim.Rel10 + pucchsim.Rel10 + prachsim.Rel10 + pdcchsim.Rel10 + pbchsim.Rel10 + mbmssim.Rel10
                        secu_knas_encrypt_eia1.Rel10 secu_kenb.Rel10 aes128_ctr_encrypt.Rel10 aes128_ctr_decrypt.Rel10 secu_knas_encrypt_eea2.Rel10
                        secu_knas.Rel10 secu_knas_encrypt_eea1.Rel10 kdf.Rel10 aes128_cmac_encrypt.Rel10 secu_knas_encrypt_eia2.Rel10)

01      01      40      Build RRH Gateway (time domain) for USRP (Rel 10)
01      01      41      Build RRH Gateway (time domain) for EXMIMO  (Rel 10)
01      01      42      Build RRH Gateway (time domain) for BLADERF  (Rel 10)
01      01      43      Build RRH Gateway (time domain) for LMSSDR (Rel 10)

01      01      50      Build RRU (NGFI) for USRP (Rel 10) w/ ETHERNET transport
01      01      51      Build RRU (NGFI) for EXMIMO  (Rel 10) w/ ETHERNET transport
01      01      52      Build RRU (NGFI) for BLADERF  (Rel 10) w/ ETHERNET transport
01      01      53      Build RRU (NGFI) for LMSSDR (Rel 10) w/ ETHERNET transport


01      02              Run OAISIM-NOS1 Rel10 (TDD + 5MHz/10MHz/20MHz + TM 1,2), and check the operation
01      02      00      Run OAISIM-NOS1 Rel10 TDD, 1 eNB + 1 UE (5 MHz/10MHz/20MHz), (TM 1,2) and search for errors, segmentation fault or exit
01      02      01      Run OAISIM-NOS1 Rel10 TDD, 1 eNB + 1 UE (5 MHz/10MHz/20MHz), (TM 1,2) in PHY_ABSTRACTION mode and search for errors
01      02      02      Run OAISIM-NOS1 Rel10 TDD, 1 eNB + 3 UEs (5 MHz/10MHz/20MHz), (TM 1,2) and search for errors, segmentation fault or exit
01      02      03      Run OAISIM-NOS1 Rel10 TDD, 1 eNB + 3 UEs (5 MHz/10MHz/20MHz), (TM 1,2) in PHY_ABSTRACTION mode and search for errors
01      02      04      Run OAISIM-NOS1 Rel10 TDD, 1 eNB + 1 UE (5 MHz/10MHz/20MHz), (TM 1,2) without PHY_ABSTRACTION mode, ping from from eNB to UE,
                        and for check for no packet losses
01      02      05      Run OAISIM-NOS1 Rel10 TDD, 1 eNB + 1 UE (5 MHz/10MHz/20MHz), (TM 1,2) in PHY_ABSTRACTION  mode, send ping from from eNB to UE,
                        and check for no packet losses

01      03              Run OAISIM-NOS1 Rel10 (FDD + 5MHz/10MHz/20MHz + TM 1,2), and check the operation
01      03      00      Run OAISIM-NOS1 Rel10 FDD, 1 eNB + 1 UE 1 eNB (5 MHz/10MHz/20MHz), (TM 1,2) and search for errors, segmentation fault or exit
01      03      01      Run OAISIM-NOS1 Rel10 FDD, 1 eNB + 1 UE (5 MHz/10MHz/20MHz), (TM 1,2) in PHY_ABSTRACTION mode and search for errors
01      03      02      Run OAISIM-NOS1 Rel10 FDD, 1 eNB + 3 UEs (5 MHz/10MHz/20MHz), (TM 1,2) and search for errors, segmentation fault or exit
01      03      03      Run OAISIM-NOS1 Rel10 FDD, 1 eNB + 3 UEs (5 MHz/10MHz/20MHz), (TM 1,2) in PHY_ABSTRACTION mode and search for errors
01      03      04      Run OAISIM-NOS1 Rel10 FDD, 1 eNB + 1 UE (5 MHz/10MHz/20MHz), (TM 1,2) without PHY_ABSTRACTION mode, ping from from eNB to UE,
                        and for check for no packet losses
01      03      05      Run OAISIM-NOS1 Rel10 FDD, 1 eNB + 1 UE (5 MHz/10MHz/20MHz), (TM 1,2) in PHY_ABSTRACTION  mode, send ping from from eNB to UE,
                        and check for no packet losses



01      04              OAISIM-NOS1 MBSFN Tests
01      04      00      Check if eMBMS procedure is not finished completely, make sure that the SIB13/MCCH have been correclty received by UEs
01      04      01      Check if eMBMS multicast/broadcast data is received, make sure that the SIB13/MCCH/MTCH have been correclty received by UEs
01      04      02      Check for eMBMS multicast/broadcast data received in fdd mode, make sure that the SIB13/MCCH/MTCH have been correctly
                        received by UEs
01      04      03      Check for eMBMS multicast/broadcast DF relaying working properly in fdd mode, make sure that the SIB13/MCCH/MTCH have been 
                        correclty received by UEs


01      50              Run PHY unitary secuirity tests
01      50      00      test_aes128_cmac_encrypt
01      50      01      test_aes128_ctr_decrypt
01      50      02      test_aes128_ctr_encrypt
01      50      03      test_secu_kenb
01      50      04      test_secu_knas
01      50      05      test_secu_knas_encrypt_eea1
01      50      06      test_secu_knas_encrypt_eea2
01      50      07      test_secu_knas_encrypt_eia1
01      50      08      test_secu_knas_encrypt_eia2
01      50      09      test_kdf
        


01      51              Run PHY simulator tests
01      51      00      dlsim test cases (Test 1: 10 MHz, R2.FDD (MCS 5), EVA5, -1dB), 
                        (Test 5: 1.4 MHz, R4.FDD (MCS 4), EVA5, 0dB (70%)),
                        (Test 6: 10 MHz, R3.FDD (MCS 15), EVA5, 6.7dB (70%)),
                        (Test 6b: 5 MHz, R3-1.FDD (MCS 15), EVA5, 6.7dB (70%)),
                        (Test 7: 5 MHz, R3-1.FDD (MCS 15), EVA5, 6.7dB (30%)),
                        (Test 7b: 5 MHz, R3-1.FDD (MCS 15), ETU70, 1.4 dB (30%)),
                        (Test 10: 5 MHz, R6.FDD (MCS 25), EVA5, 17.4 dB (70%)),
                        (Test 10b: 5 MHz, R6-1.FDD (MCS 24,18 PRB), EVA5, 17.5dB (70%)),
                        (Test 11: 10 MHz, R7.FDD (MCS 25), EVA5, 17.7dB (70%))
		        (TM2 Test 1 10 MHz, R.11 FDD (MCS 14), EVA5, 6.8 dB (70%)),
		        (TM2 Test 1b 20 MHz, R.11-2 FDD (MCS 13), EVA5, 5.9 dB (70%)),
01      51      01      ulsim Test cases. (Test 1, 5 MHz, FDD (MCS 5), AWGN, 6dB), 
                        (Test 2, 5 MHz, FDD (MCS 16), AWGN , 12dB (70%)),
                        (Test 3, 10 MHz, R3.FDD (MCS 5), AWGN, 6dB (70%)),
                        (Test 4, 10 MHz, R3-1.FDD (MCS 16), AWGN, 12dB (70%)),
                        (Test 5, 20 MHz, FDD (MCS 5), AWGN, 6dB (70%)),
                        (Test 6, 20 MHz, FDD (MCS 16), AWGN, 12 dB (70%))
01      51      02      pucchsim (TBD)
01      51      03      prachsim (TBD)
01      51      04      pdcchsim (TBD)
01      51      05      pbchsim (TBD)
01      51      06      mbmssim (TBD)
01      51      10      dlsim_tm4 test cases (Test 1: 10 MHz, R2.FDD (MCS 5), EVA5, -1dB), 
                        (Test 5: 1.4 MHz, R4.FDD (MCS 4), EVA5, 0dB (70%)),
                        (Test 6: 10 MHz, R3.FDD (MCS 15), EVA5, 6.7dB (70%)),
                        (Test 6b: 5 MHz, R3-1.FDD (MCS 15), EVA5, 6.7dB (70%)),
                        (Test 7: 5 MHz, R3-1.FDD (MCS 15), EVA5, 6.7dB (30%)),
                        (Test 7b: 5 MHz, R3-1.FDD (MCS 15), ETU70, 1.4 dB (30%)),
                        (Test 10: 5 MHz, R6.FDD (MCS 25), EVA5, 17.4 dB (70%)),
                        (Test 10b: 5 MHz, R6-1.FDD (MCS 24,18 PRB), EVA5, 17.5dB (70%)),
                        (Test 11: 10 MHz, R7.FDD (MCS 25), EVA5, 17.7dB (70%))
		        (TM2 Test 1 10 MHz, R.11 FDD (MCS 14), EVA5, 6.8 dB (70%)),
		        (TM2 Test 1b 20 MHz, R.11-2 FDD (MCS 13), EVA5, 5.9 dB (70%)),




01      55              lte-softmodem tests with USRP B210 RF as eNB and ALU EPC w/ Bandrich COTS UE for 1TX/1RX (TM1), 2TX/2RX (TM2)
01      55      00      Band 7 FDD 5MHz UL Throughput (UDP) for 300 sec for 1TX/1RX 
01      55      01      Band 7 FDD 10MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      55      02      Band 7 FDD 20MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      55      03      Band 7 FDD 5MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      55      04      Band 7 FDD 10MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      55      05      Band 7 FDD 20MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      55      06      Band 7 FDD 5MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      55      07      Band 7 FDD 10MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      55      08      Band 7 FDD 20MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      55      09      Band 7 FDD 5MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      55      10      Band 7 FDD 10MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      55      11      Band 7 FDD 20MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      55      12      Band 7 FDD 5MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      55      13      Band 7 FDD 10MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      55      14      Band 7 FDD 20MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      55      15      Band 7 FDD 5MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      55      16      Band 7 FDD 10MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      55      17      Band 7 FDD 20MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      55      18      Band 7 FDD 5MHz UL Throughput (TCP) for 300 sec for 2TX/2RX  (TM2)
01      55      19      Band 7 FDD 10MHz UL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
01      55      20      Band 7 FDD 20MHz UL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
01      55      21      Band 7 FDD 5MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
01      55      22      Band 7 FDD 10MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
01      55      23      Band 7 FDD 20MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)

01      56              lte-softmodem tests with USRP B210  RF as eNB and  OAI EPC (eNB and EPC are on same machines) w/ Bandrich COTS UE
01      56      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      56      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      56      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      56      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      56      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      56      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX

01      57              lte-softmodem tests with USRP B210 RF as eNB and OAI EPC (eNB and EPC are on different machines) w/ Bandrich COTS UE
01      57      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      57      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      57      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      57      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      57      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      57      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX

01      58               lte-softmodem tests with USRP X310 RF as eNB and ALU EPC w/ Bandrich COTS UE for 1TX/1RX (TM1), 2TX/2RX(TM2)
01      58      00      Band 7 FDD 5MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      58      01      Band 7 FDD 10MHz UL Throughput (UDP)  for 300 sec for 1TX/1RX
01      58      02      Band 7 FDD 20MHz UL Throughput (UDP)  for 300 sec for 1TX/1RX
01      58      03      Band 7 FDD 5MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      58      04      Band 7 FDD 10MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      58      05      Band 7 FDD 20MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      58      06      Band 7 FDD 5MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      58      07      Band 7 FDD 10MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      58      08      Band 7 FDD 20MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      58      09      Band 7 FDD 5MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      58      10      Band 7 FDD 10MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      58      11      Band 7 FDD 20MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      58      12      Band 7 FDD 5MHz UL Throughput (UDP) for 300 sec for 2TX/2RX  (TM2)  
01      58      13      Band 7 FDD 10MHz UL Throughput (UDP)  for 300 sec for 2TX/2RX  (TM2)
01      58      14      Band 7 FDD 20MHz UL Throughput (UDP)  for 300 sec for 2TX/2RX  (TM2)
01      58      15      Band 7 FDD 5MHz DL Throughput (UDP) for 300 sec for 2TX/2RX  (TM2)
01      58      16      Band 7 FDD 10MHz DL Throughput (UDP) for 300 sec for 2TX/2RX  (TM2)
01      58      17      Band 7 FDD 20MHz DL Throughput (UDP) for 300 sec for 2TX/2RX  (TM2)
01      58      18      Band 7 FDD 5MHz UL Throughput (TCP) for 300 sec for 2TX/2RX  (TM2)
01      58      19      Band 7 FDD 10MHz UL Throughput (TCP) for 300 sec for 2TX/2RX  (TM2)
01      58      20      Band 7 FDD 20MHz UL Throughput (TCP) for 300 sec for 2TX/2RX  (TM2)
01      58      21      Band 7 FDD 5MHz DL Throughput (TCP) for 300 sec for 2TX/2RX  (TM2)
01      58      22      Band 7 FDD 10MHz DL Throughput (TCP) for 300 sec for 2TX/2RX  (TM2)
01      58      23      Band 7 FDD 20MHz DL Throughput (TCP) for 300 sec for 2TX/2RX  (TM2)


01      59              lte-softmodem tests with USRP X310  RF as eNB and  OAI EPC (eNB and EPC are on same machines) w/ Bandrich COTS UE

01      60              lte-softmodem tests with USRP X310 RF as eNB and OAI EPC (eNB and EPC are on different machines) w/ Bandrich COTS UE
01      60      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      60      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      60      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      60      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      60      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      60      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX

01      61              lte-softmodem tests with EXMIMO RF as eNB and ALU EPC w/ Bandrich COTS UE for 1TX/1RX, 2TX/2RX
01      61      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      61      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      61      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      61      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      61      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      61      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX

01      62              lte-softmodem tests with EXMIMO RF as eNB and OAI EPC (eNB and EPC are on same machines) w/ Bandrich COTS UE

01      63              lte-softmodem tests with EXMIMO RF as eNB and  OAI EPC (eNB and EPC are on different machines) w/ Bandrich COTS UE
01      63      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      63      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      63      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      63      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      63      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      63      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX

01      65              lte-softmodem tests with BladeRF RF as eNB and ALU EPC w/ Bandrich COTS UE for 1TX/1RX
01      65      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      65      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      65      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      65      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      65      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      65      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX


01      70              lte-softmodem tests with SoDeRa RF as eNB and ALU EPC w/ Bandrich COTS UE for TX/1RX
01      70      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      70      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      70      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      70      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      70      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      70      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX


01      75              lte-softmodem + RRU (NGFI IF4P5, RAW) tests with B210 RF as eNB and ALU EPC w/ Bandrich COTS UE for TX/1RX
01      75      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      75      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      75      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      75      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      75      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      75      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX

01      76              lte-softmodem + RRU (NGFI IF4P5, UDP) tests with B210 RF as eNB and ALU EPC w/ Bandrich COTS UE for TX/1RX
01      76      00      Band 7 FDD 5MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      76      01      Band 7 FDD 10MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      76      02      Band 7 FDD 20MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      76      03      Band 7 FDD 5MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      76      04      Band 7 FDD 10MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      76      05      Band 7 FDD 20MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      76      06      Band 7 FDD 5MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      76      07      Band 7 FDD 10MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      76      08      Band 7 FDD 20MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      76      09      Band 7 FDD 5MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      76      10      Band 7 FDD 10MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      76      11      Band 7 FDD 20MHz DL Throughput (TCP) for 300 sec for 1TX/1RX

01      80              lte-softmodem + RRU (NGFI) tests with BladeRF RF as eNB and ALU EPC w/ Bandrich COTS UE for TX/1RX
01      80      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      80      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      80      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      80      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      80      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      80      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX

01      85              lte-softmodem + RRU (NGFI) tests with USRP X310 RF as eNB and ALU EPC w/ Bandrich COTS UE for TX/1RX
01      85      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
01      85      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
01      85      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
01      85      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
01      85      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
01      85      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX

01      86              lte-softmodem tests with USRP B210 RF as eNB and ALU EPC w/ Huawei e3276 COTS UE for 1TX/1RX (TM1), 2TX/2RX (TM2)
01      86      00      Band 38 TDD 5MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      86      01      Band 38 TDD 10MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      86      02      Band 38 TDD 20MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
01      86      03      Band 38 TDD 5MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      86      04      Band 38 TDD 10MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      86      05      Band 38 TDD 20MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
01      86      06      Band 38 TDD 5MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      86      07      Band 38 TDD 10MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      86      08      Band 38 TDD 20MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      86      09      Band 38 TDD 5MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      86      10      Band 38 TDD 10MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      86      11      Band 38 TDD 20MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
01      86      12      Band 38 TDD 5MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      86      13      Band 38 TDD 10MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      86      14      Band 38 TDD 20MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
01      86      15      Band 38 TDD 5MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      86      16      Band 38 TDD 10MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      86      17      Band 38 TDD 20MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
01      86      18      Band 38 TDD 5MHz UL Throughput (TCP) for 300 sec for 2TX/2RX  (TM2)
01      86      19      Band 38 TDD 10MHz UL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
01      86      20      Band 38 TDD 20MHz UL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
01      86      21      Band 38 TDD 5MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
01      86      22      Band 38 TDD 10MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
01      86      23      Band 38 TDD 20MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)

02      55              lte-softmodem tests with USRP B210 RF as eNB and ALU EPC w/ Sony Experia M4 COTS UE for 1TX/1RX and 2TX/2RX (TM2)
02      55      00      Band 7 FDD 5MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
02      55      01      Band 7 FDD 10MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
02      55      02      Band 7 FDD 20MHz UL Throughput (UDP) for 300 sec for 1TX/1RX
02      55      03      Band 7 FDD 5MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
02      55      04      Band 7 FDD 10MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
02      55      05      Band 7 FDD 20MHz DL Throughput (UDP) for 300 sec for 1TX/1RX
02      55      06      Band 7 FDD 5MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
02      55      07      Band 7 FDD 10MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
02      55      08      Band 7 FDD 20MHz UL Throughput (TCP) for 300 sec for 1TX/1RX
02      55      09      Band 7 FDD 5MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
02      55      10      Band 7 FDD 10MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
02      55      11      Band 7 FDD 20MHz DL Throughput (TCP) for 300 sec for 1TX/1RX
02      55      12      Band 7 FDD 5MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
02      55      13      Band 7 FDD 10MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
02      55      14      Band 7 FDD 20MHz UL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
02      55      15      Band 7 FDD 5MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
02      55      16      Band 7 FDD 10MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
02      55      17      Band 7 FDD 20MHz DL Throughput (UDP) for 300 sec for 2TX/2RX (TM2)
02      55      18      Band 7 FDD 5MHz UL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
02      55      19      Band 7 FDD 10MHz UL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
02      55      20      Band 7 FDD 20MHz UL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
02      55      21      Band 7 FDD 5MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
02      55      22      Band 7 FDD 10MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2)
02      55      23      Band 7 FDD 20MHz DL Throughput (TCP) for 300 sec for 2TX/2RX (TM2) 

02      57              lte-softmodem tests with USRP B210 RF as eNB and OAI EPC (eNB and EPC are on different machines) w/ OAI UE
02      57      00      Band 7 FDD 5MHz UL Throughput for 300 sec for 1TX/1RX
02      57      01      Band 7 FDD 10MHz UL Throughput for 300 sec for 1TX/1RX
02      57      02      Band 7 FDD 20MHz UL Throughput for 300 sec for 1TX/1RX
02      57      03      Band 7 FDD 5MHz DL Throughput for 300 sec for 1TX/1RX
02      57      04      Band 7 FDD 10MHz DL Throughput for 300 sec for 1TX/1RX
02      57      05      Band 7 FDD 20MHz DL Throughput for 300 sec for 1TX/1RX


01      64              lte-softmodem-noS1 tests

02                      Functional test case

03                      Autotests specific to OAI UE

04                      Failure test case 
 
05                      Performance test case 


#TODO: Add test cases for 10,20 MHz
#TODO: Add test cases for TDD/FDD 
#TODO: Test and compile seperately for Rel8/Rel10
#TODO: Case03.py eMBMS test case
