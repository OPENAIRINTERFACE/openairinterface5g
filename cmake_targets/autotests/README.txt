OAI Test PLAN
#UNDER CONSTRUCTION. Not correct at the moment

Obj.#   Case#   Test#	Description

01                      pre-commit test case
01      01              Build OAI 
01      01      01      Build oaisim.Rel8
01      01      02      Build oaisim.Rel8 + network device driver(nasmesh_fix)	
01      01      03      Build (lte-softmodem.Rel8.EXMIMO + lte-softmodem.Rel10.EXMIMO + lte-softmodem.Rel10.USRP)	
01      01      04      Build (dlsim.Rel10 + ulsim.Rel10 + pucchsim.Rel10 + prachsim.Rel10 + pdcchsim.Rel10 + pbchsim.Rel10 + mbmssim.Rel10
                        secu_knas_encrypt_eia1.Rel10 secu_kenb.Rel10 aes128_ctr_encrypt.Rel10 aes128_ctr_decrypt.Rel10 secu_knas_encrypt_eea2.Rel10
                        secu_knas.Rel10 secu_knas_encrypt_eea1.Rel10 kdf.Rel10 aes128_cmac_encrypt.Rel10 secu_knas_encrypt_eia2.Rel10)
01      01      06      Build oaisim.Rel8 + ITTI
01      01      07      Build oaisim.Rel10 
01      01      08      Build oaisim.Rel10 + ITTI
01      01      20      Build Nasmesh
01      01      30      Build RRH Gateway


01      02              Run OAISIM Rel10 (TDD + 5MHz + Transmission mode 1), and check the operation
01      02      00      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (5 MHz) and search for errors, segmentation fault or exit
01      02      01      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      02      02      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (5 MHz) and search for errors, segmentation fault or exit
01      02      03      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      02      04      Run OAI Rel10 TDD, 1 eNB + 1 UE (5 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      02      05      Run OAI Rel10 TDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check that there is no packet losses

01      03              Run OAISIM Rel10 (FDD + 5MHz + Tranmission mode 1), and check the operation
01      03      00      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (5 MHz) and search for errors, segmentation fault or exit
01      03      01      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      03      02      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (5 MHz) and search for errors, segmentation fault or exit
01      03      03      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      03      04      Run OAI Rel10 FDD, 1 eNB + 1 UE (5 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      03      05      Run OAI Rel10 FDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check that there is no packet losses

01      04              Run OAISIM Rel10 (TDD + 10MHz + Tranmission mode 1), and check the operation
01      04      00      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (10 MHz) and search for errors, segmentation fault or exit
01      04      01      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (10 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      04      02      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (10 MHz) and search for errors, segmentation fault or exit
01      04      03      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (10 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      04      04      Run OAI Rel10 TDD, 1 eNB + 1 UE (10 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      04      05      Run OAI Rel10 TDD, 1 eNB + 1 UE (10 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check that there is no packet losses

01      05              Run OAISIM Rel10 (FDD + 10MHz + Tranmission mode 1), and check the operation
01      05      00      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (10 MHz) and search for errors, segmentation fault or exit
01      05      01      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (10 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      05      02      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (10 MHz) and search for errors, segmentation fault or exit
01      05      03      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (10 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      05      04      Run OAI Rel10 FDD, 1 eNB + 1 UE (10 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      05      05      Run OAI Rel10 FDD, 1 eNB + 1 UE (10 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check that there is no packet losses

01      06              Run OAISIM Rel10 (TDD + 20MHz + Tranmission mode 1), and check the operation
01      06      00      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (20MHz) and search for errors, segmentation fault or exit
01      06      01      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (20MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      06      02      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (20MHz) and search for errors, segmentation fault or exit
01      06      03      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (20MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      06      04      Run OAI Rel10 TDD, 1 eNB + 1 UE (20 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      06      05      Run OAI Rel10 TDD, 1 eNB + 1 UE (20 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check that there is no packet losses

01      07              Run OAISIM Rel10 (FDD + 20MHz + Tranmission mode 1), and check the operation
01      07      00      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (20MHz) and search for errors, segmentation fault or exit
01      07      01      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (20MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      07      02      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (20MHz) and search for errors, segmentation fault or exit
01      07      03      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (20MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      07      04      Run OAI Rel10 FDD, 1 eNB + 1 UE (20 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      07      05      Run OAI Rel10 FDD, 1 eNB + 1 UE (20 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check that there is no packet losses

01      08              Run OAISIM Rel10 (TDD + 5 MHz + Tranmission mode 1), and check the operation
01      08      00      Run OAISIM Rel10 TDD, 2 eNB + 1 UE per eNB (5 MHz) and search for errors, segmentation fault or exit
01      08      01      Run OAISIM Rel10 TDD, 2 eNB + 1 UE per eNB  (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      08      02      Run OAISIM Rel10 TDD, 2 eNB + 3 UEs per eNB  (5 MHz) and search for errors, segmentation fault or exit
01      08      03      Run OAISIM Rel10 TDD, 2 eNB + 3 UEs per eNB (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      08      04      Run OAI Rel10 TDD, 2 eNB + 1 UE per eNB (5 MHz) without abstraction mode, send ping from from eNB to UE, and check there is no packet losses
01      08      05      Run OAI Rel10 TDD, 2 eNB + 1 UE per eNB (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check there is no packet losses

01      09              Run OAISIM Rel10 (FDD + 5 MHz + Tranmission mode 1), and check the operation
01      09      00      Run OAISIM Rel10 FDD, 2 eNB + 1 UE per eNB (5 MHz) and search for errors, segmentation fault or exit
01      09      01      Run OAISIM Rel10 FDD, 2 eNB + 1 UE per eNB  (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      09      02      Run OAISIM Rel10 FDD, 2 eNB + 3 UEs per eNB  (5 MHz) and search for errors, segmentation fault or exit
01      09      03      Run OAISIM Rel10 FDD, 2 eNB + 3 UEs per eNB (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      09      04      Run OAI Rel10 FDD, 2 eNB + 1 UE per eNB (5 MHz) without abstraction mode, send ping from from eNB to UE, and check there is no packet losses
01      09      05      Run OAI Rel10 FDD, 2 eNB + 1 UE per eNB (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check there is no packet losses


01      10              Run OAISIM Rel10 (TDD + 5MHz + Transmission mode 2), and check the operation
01      10      00      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (5 MHz) and search for errors, segmentation fault or exit
01      10      01      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      10      02      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (5 MHz) and search for errors, segmentation fault or exit
01      10      03      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      10      04      Run OAI Rel10 TDD, 1 eNB + 1 UE (5 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      10      05      Run OAI Rel10 TDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check that there is no packet losses

01      11              Run OAISIM Rel10 (FDD + 5MHz + Tranmission mode 2), and check the operation
01      11      00      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (5 MHz) and search for errors, segmentation fault or exit
01      11      01      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      11      02      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (5 MHz) and search for errors, segmentation fault or exit
01      11      03      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      11      04      Run OAI Rel10 FDD, 1 eNB + 1 UE (5 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      11      05      Run OAI Rel10 FDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check that there is no packet losses

01      12              Run OAISIM Rel10 (TDD + 10MHz + Tranmission mode 2), and check the operation
01      12      00      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (10 MHz) and search for errors, segmentation fault or exit
01      12      01      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (10 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      12      02      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (10 MHz) and search for errors, segmentation fault or exit
01      12      03      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (10 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      12      04      Run OAI Rel10 TDD, 1 eNB + 1 UE (10 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      12      05      Run OAI Rel10 TDD, 1 eNB + 1 UE (10 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check that there is no packet losses

01      13              Run OAISIM Rel10 (FDD + 10MHz + Tranmission mode 2), and check the operation
01      13      00      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (10 MHz) and search for errors, segmentation fault or exit
01      13      01      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (10 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      13      02      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (10 MHz) and search for errors, segmentation fault or exit
01      13      03      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (10 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      13      04      Run OAI Rel10 FDD, 1 eNB + 1 UE (10 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      13      05      Run OAI Rel10 FDD, 1 eNB + 1 UE (10 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check that there is no packet losses

01      14              Run OAISIM Rel10 (TDD + 20MHz + Tranmission mode 2), and check the operation
01      14      00      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (20MHz) and search for errors, segmentation fault or exit
01      14      01      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (20MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      14      02      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (20MHz) and search for errors, segmentation fault or exit
01      14      03      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (20MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      14      04      Run OAI Rel10 TDD, 1 eNB + 1 UE (20 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      14      05      Run OAI Rel10 TDD, 1 eNB + 1 UE (20 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check that there is no packet losses

01      15              Run OAISIM Rel10 (FDD + 20MHz + Tranmission mode 2), and check the operation
01      15      00      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (20MHz) and search for errors, segmentation fault or exit
01      15      01      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (20MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      15      02      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (20MHz) and search for errors, segmentation fault or exit
01      15      03      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (20MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      15      04      Run OAI Rel10 FDD, 1 eNB + 1 UE (20 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      15      05      Run OAI Rel10 FDD, 1 eNB + 1 UE (20 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check that there is no packet losses

01      16              Run OAISIM Rel10 (TDD + 5 MHz + Tranmission mode 2), and check the operation
01      16      00      Run OAISIM Rel10 TDD, 2 eNB + 1 UE per eNB (5 MHz) and search for errors, segmentation fault or exit
01      16      01      Run OAISIM Rel10 TDD, 2 eNB + 1 UE per eNB  (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      16      02      Run OAISIM Rel10 TDD, 2 eNB + 3 UEs per eNB  (5 MHz) and search for errors, segmentation fault or exit
01      16      03      Run OAISIM Rel10 TDD, 2 eNB + 3 UEs per eNB (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      16      04      Run OAI Rel10 TDD, 2 eNB + 1 UE per eNB (5 MHz) without abstraction mode, send ping from from eNB to UE, and check there is no packet losses
01      16      05      Run OAI Rel10 TDD, 2 eNB + 1 UE per eNB (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check there is no packet losses

01      17              Run OAISIM Rel10 (FDD + 5 MHz + Tranmission mode 2), and check the operation
01      17      00      Run OAISIM Rel10 FDD, 2 eNB + 1 UE per eNB (5 MHz) and search for errors, segmentation fault or exit
01      17      01      Run OAISIM Rel10 FDD, 2 eNB + 1 UE per eNB  (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      17      02      Run OAISIM Rel10 FDD, 2 eNB + 3 UEs per eNB  (5 MHz) and search for errors, segmentation fault or exit
01      17      03      Run OAISIM Rel10 FDD, 2 eNB + 3 UEs per eNB (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      17      04      Run OAI Rel10 FDD, 2 eNB + 1 UE per eNB (5 MHz) without abstraction mode, send ping from from eNB to UE, and check there is no packet losses
01      17      05      Run OAI Rel10 FDD, 2 eNB + 1 UE per eNB (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check there is no packet losses

01      18              Run OAISIM Rel10 (TDD + 5MHz + Transmission mode 5), and check the operation
01      18      00      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (5 MHz) and search for errors, segmentation fault or exit
01      18      01      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      18      02      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (5 MHz) and search for errors, segmentation fault or exit
01      18      03      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      18      04      Run OAI Rel10 TDD, 1 eNB + 1 UE (5 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      18      05      Run OAI Rel10 TDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check that there is no packet losses

01      19              Run OAISIM Rel10 (FDD + 5MHz + Tranmission mode 5), and check the operation
01      19      00      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (5 MHz) and search for errors, segmentation fault or exit
01      19      01      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      19      02      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (5 MHz) and search for errors, segmentation fault or exit
01      19      03      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      19      04      Run OAI Rel10 FDD, 1 eNB + 1 UE (5 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      19      05      Run OAI Rel10 FDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check that there is no packet losses


01      26              Run OAISIM Rel10 (TDD + 5MHz + Transmission mode 6), and check the operation
01      26      00      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (5 MHz) and search for errors, segmentation fault or exit
01      26      01      Run OAISIM Rel10 TDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      26      02      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (5 MHz) and search for errors, segmentation fault or exit
01      26      03      Run OAISIM Rel10 TDD, 1 eNB + 3 UEs (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      26      04      Run OAI Rel10 TDD, 1 eNB + 1 UE (5 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      26      05      Run OAI Rel10 TDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to  UE, and check that there is no packet losses

01      27              Run OAISIM Rel10 (FDD + 5MHz + Tranmission mode 6), and check the operation
01      27      00      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (5 MHz) and search for errors, segmentation fault or exit
01      27      01      Run OAISIM Rel10 FDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      27      02      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (5 MHz) and search for errors, segmentation fault or exit
01      27      03      Run OAISIM Rel10 FDD, 1 eNB + 3 UEs (5 MHz) in PHY_ABSTRACTION mode and search for errors, segmentation fault or exit
01      27      04      Run OAI Rel10 FDD, 1 eNB + 1 UE (5 MHz) without abstraction mode, send ping from from eNB to UE, and check that there is no packet losses
01      27      05      Run OAI Rel10 FDD, 1 eNB + 1 UE (5 MHz) in PHY_ABSTRACTION  mode, send ping from from eNB to UE, and check that there is no packet losses


01      50              Run PHY unitary secuirity tests
01      50      00      aes128_cmac_encrypt
01      50      01      aes128_ctr_decrypt
01      50      02      aes128_ctr_encrypt
01      50      03      secu_kenb
01      50      04      secu_knas
01      50      05      secu_knas_encrypt_eea1
01      50      06      secu_knas_encrypt_eea2
01      50      07      secu_knas_encrypt_eia1
01      50      08      secu_knas_encrypt_eia2
01      50      09      kdf
        


01      51              Run PHY simulator tests
01      51      00      dlsim
01      51      00      ulsim
01      51      00      pucchsim
01      51      00      prachsim
01      51      00      pdcchsim
01      51      00      pbchsim
01      51      00      mbmssim


01      02      01      Run OAI Rel8, and search for execution errors
01      02      02      Run OAI Rel8 in abstraction mode and check that RRC proc is finished completely for the configured number of eNB and UE
01      02      03      Run OAI Rel8 in abstraction mode, send ping from from one eNB to each UE, and check that there is no packet losses
01      02      04      Run OAI Rel8 with full PHY, and check that the RRC proc for eNBsxUEs
01      02      05      Run OAI Rel8 with full PHY in FDD mode, and check that the RRC proc for eNBsxUEs

01      03              Run OAI Rel10, and check the operation
01      03      00      Run OAI Rel10, and search for segmentation fault or exit
01      03      01      Run OAI Rel10, and search for execution errors
01      03      02      Run OAI Rel10 in abstraction mode, and check the RRC proc for eNBsxUEs	
01      03      03      Run OAI Rel10 in full phy mode, and check the RRC proc for eNBsxUEs
01      03      04      Run OAI Rel10 in full phy mode in FDD mode, and check the RRC proc for eNBsxUEs
01      03      05      Run OAI Rel10 with eMBMS enabled, and check the SIB13 and MCCH
01      03      06      Run OAI Rel10 with eMBMS enabled, and check the MTCH
01      03      07      Run OAI Rel10 with eMBMS enabled and FDD mode, and check the MTCH

02                      Functional test case

03                      Non-Functional test case

04                      Failure test case 
 
05                      Performance test case 


#TODO: Add test cases for 10,20 MHz
#TODO: Add test cases for TDD/FDD 
#TODO: Test and compile seperately for Rel8/Rel10
#TODO: Case03.py eMBMS test case


