OAI Test PLAN

Obj.#   Case#   Test#   Description

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
01      51      02      ldpc Test cases. (Test1: block length = 3872),
                        (Test2: block length = 4224),
                        (Test3: block length = 4576),
                        (Test4: block length = 4928),
                        (Test5: block length = 5280),
                        (Test6: block length = 5632),
                        (Test7: block length = 6336),
                        (Test8: block length = 7040),
                        (Test9: block length = 7744),
                        (Test10: block length = 8448)
01      51      03      polartest Test cases. (Test1: PBCH polar test),
                        (Test2: DCI polar test)
01      51      04      nr_pbchsim Test cases. (Test1: PBCH-only, 106 PRB),
                        (Test2: PBCH and synchronization, 106PBR),
                        (Test3: PBCH-only, 217 PRB),
                        (Test4: PBCH and synchronization, 217 RPB),
                        (Test5: PBCH-only, 273 PRB),
                        (Test6: PBCH and synchronization, 273 PRB)
01      51      05      nr_dlsim Test cases. (Test1: 106 PRB),
                        (Test2: 217 PRB),
                        (Test3: 273 PRB)
01      51      06      nr_dlschsim Test cases. (Test1: 106 PRB),
                        (Test2: 217 PRB),
                        (Test3: 273 PRB)
01      51      07      shortblocktest Test cases. (Test1: 3 bits),
                        (Test2: 6 bits),
                        (Test3: 7 bits),
                        (Test4: 11 bits)
01      51      08      nr_ulschsim Test cases. (Test1: 106 PRB),
                        (Test2: 217 PRB),
                        (Test3: 273 PRB)
01      51      09      nr_pucchsim Test cases. (Test1: Format 0 ACK miss 106 PRB),
                        (Test2: Format 1 ACK miss 106 PRB),
                        (Test3: Format 1 ACK miss 273 PRB),
                        (Test4: Format 1 NACKtoACK 106 PRB)
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
01      51      11      nr_ulsim Test cases. (Test1: MCS 9 106 PRBs),
                        (Test2: MCS 16 50 PRBs),
                        (Test3: MCS 28 50 PRBs),
                        (Test4: MCS 9 217 PRBs),
                        (Test5: MCS 9 273 PRBs)
01      51      12      nr_prachsim Test cases.(Test1: 106 PRBs),
                        (Test2: 217 PRBs),
                        (Test3: 273 PRBs)
