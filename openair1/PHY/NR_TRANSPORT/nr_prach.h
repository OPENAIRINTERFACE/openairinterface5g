/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file PHY/LTE_TRANSPORT/prach_common.c
 * \brief Common routines for UE/eNB PRACH physical channel V8.6 2009-03
 * \author Agustin Mico Pereperez
 * \date 2018
 * \version 0.1
 * \company 
 * \email: 
 * \note
 * \warning
 */

int16_t nr_ru[2*839]; // quantized roots of unity
uint32_t nr_ZC_inv[839]; // multiplicative inverse for roots u
uint16_t nr_du[838];

/*************************************
* The following tables defined for NR
**************************************/
// Table 6.3.3.1-5 (38.211) NCS for preamble formats with delta_f_RA = 1.25 KHz
uint16_t NCS_unrestricted_delta_f_RA_125[16] = {0,13,15,18,22,26,32,38,46,59,76,93,119,167,279,419};
uint16_t NCS_restricted_TypeA_delta_f_RA_125[15]   = {15,18,22,26,32,38,46,55,68,82,100,128,158,202,237}; // high-speed case set Type A
uint16_t NCS_restricted_TypeB_delta_f_RA_125[13]   = {15,18,22,26,32,38,46,55,68,82,100,118,137}; // high-speed case set Type B

// Table 6.3.3.1-6 (38.211) NCS for preamble formats with delta_f_RA = 5 KHz
uint16_t NCS_unrestricted_delta_f_RA_5[16] = {0,13,26,33,38,41,49,55,64,76,93,119,139,209,279,419};
uint16_t NCS_restricted_TypeA_delta_f_RA_5[16]   = {36,57,72,81,89,94,103,112,121,132,137,152,173,195,216,237}; // high-speed case set Type A
uint16_t NCS_restricted_TypeB_delta_f_RA_5[14]   = {36,57,60,63,65,68,71,77,81,85,97,109,122,137}; // high-speed case set Type B

// Table 6.3.3.1-7 (38.211) NCS for preamble formats with delta_f_RA = 15 * 2mu KHz where mu = {0,1,2,3}
uint16_t NCS_unrestricted_delta_f_RA_15[16] = {0,2,4,6,8,10,12,13,15,17,19,23,27,34,46,69};

//Table 6.3.3.1-3: Mapping from logical index i to sequence number u for preamble formats with L_RA = 839
uint16_t prach_root_sequence_map_0_3[838] = {
129, 710, 140, 699, 120, 719, 210, 629, 168, 671, 84 , 755, 105, 734, 93 , 746, 70 , 769, 60 , 779,
2  , 837, 1  , 838, 56 , 783, 112, 727, 148, 691, 80 , 759, 42 , 797, 40 , 799, 35 , 804, 73 , 766,
146, 693, 31 , 808, 28 , 811, 30 , 809, 27 , 812, 29 , 810, 24 , 815, 48 , 791, 68 , 771, 74 , 765,
178, 661, 136, 703, 86 , 753, 78 , 761, 43 , 796, 39 , 800, 20 , 819, 21 , 818, 95 , 744, 202, 637,
190, 649, 181, 658, 137, 702, 125, 714, 151, 688, 217, 622, 128, 711, 142, 697, 122, 717, 203, 636,
118, 721, 110, 729, 89 , 750, 103, 736, 61 , 778, 55 , 784, 15 , 824, 14 , 825, 12 , 827, 23 , 816,
34 , 805, 37 , 802, 46 , 793, 207, 632, 179, 660, 145, 694, 130, 709, 223, 616, 228, 611, 227, 612,
132, 707, 133, 706, 143, 696, 135, 704, 161, 678, 201, 638, 173, 666, 106, 733, 83 , 756, 91 , 748,
66 , 773, 53 , 786, 10 , 829, 9  , 830, 7  , 832, 8  , 831, 16 , 823, 47 , 792, 64 , 775, 57 , 782,
104, 735, 101, 738, 108, 731, 208, 631, 184, 655, 197, 642, 191, 648, 121, 718, 141, 698, 149, 690,
216, 623, 218, 621, 152, 687, 144, 695, 134, 705, 138, 701, 199, 640, 162, 677, 176, 663, 119, 720,
158, 681, 164, 675, 174, 665, 171, 668, 170, 669, 87 , 752, 169, 670, 88 , 751, 107, 732, 81 , 758,
82 , 757, 100, 739, 98 , 741, 71 , 768, 59 , 780, 65 , 774, 50 , 789, 49 , 790, 26 , 813, 17 , 822,
13 , 826, 6  , 833, 5  , 834, 33 , 806, 51 , 788, 75 , 764, 99 , 740, 96 , 743, 97 , 742, 166, 673,
172, 667, 175, 664, 187, 652, 163, 676, 185, 654, 200, 639, 114, 725, 189, 650, 115, 724, 194, 645,
195, 644, 192, 647, 182, 657, 157, 682, 156, 683, 211, 628, 154, 685, 123, 716, 139, 700, 212, 627,
153, 686, 213, 626, 215, 624, 150, 689, 225, 614, 224, 615, 221, 618, 220, 619, 127, 712, 147, 692,
124, 715, 193, 646, 205, 634, 206, 633, 116, 723, 160, 679, 186, 653, 167, 672, 79 , 760, 85 , 754,
77 , 762, 92 , 747, 58 , 781, 62 , 777, 69 , 770, 54 , 785, 36 , 803, 32 , 807, 25 , 814, 18 , 821,
11 , 828, 4  , 835, 3  , 836, 19 , 820, 22 , 817, 41 , 798, 38 , 801, 44 , 795, 52 , 787, 45 , 794,
63 , 776, 67 , 772, 72 , 767, 76 , 763, 94 , 745, 102, 737, 90 , 749, 109, 730, 165, 674, 111, 728,
209, 630, 204, 635, 117, 722, 188, 651, 159, 680, 198, 641, 113, 726, 183, 656, 180, 659, 177, 662,
196, 643, 155, 684, 214, 625, 126, 713, 131, 708, 219, 620, 222, 617, 226, 613, 230, 609, 232, 607,
262, 577, 252, 587, 418, 421, 416, 423, 413, 426, 411, 428, 376, 463, 395, 444, 283, 556, 285, 554,
379, 460, 390, 449, 363, 476, 384, 455, 388, 451, 386, 453, 361, 478, 387, 452, 360, 479, 310, 529,
354, 485, 328, 511, 315, 524, 337, 502, 349, 490, 335, 504, 324, 515, 323, 516, 320, 519, 334, 505,
359, 480, 295, 544, 385, 454, 292, 547, 291, 548, 381, 458, 399, 440, 380, 459, 397, 442, 369, 470,
377, 462, 410, 429, 407, 432, 281, 558, 414, 425, 247, 592, 277, 562, 271, 568, 272, 567, 264, 575,
259, 580, 237, 602, 239, 600, 244, 595, 243, 596, 275, 564, 278, 561, 250, 589, 246, 593, 417, 422,
248, 591, 394, 445, 393, 446, 370, 469, 365, 474, 300, 539, 299, 540, 364, 475, 362, 477, 298, 541,
312, 527, 313, 526, 314, 525, 353, 486, 352, 487, 343, 496, 327, 512, 350, 489, 326, 513, 319, 520,
332, 507, 333, 506, 348, 491, 347, 492, 322, 517, 330, 509, 338, 501, 341, 498, 340, 499, 342, 497,
301, 538, 366, 473, 401, 438, 371, 468, 408, 431, 375, 464, 249, 590, 269, 570, 238, 601, 234, 605,
257, 582, 273, 566, 255, 584, 254, 585, 245, 594, 251, 588, 412, 427, 372, 467, 282, 557, 403, 436,
396, 443, 392, 447, 391, 448, 382, 457, 389, 450, 294, 545, 297, 542, 311, 528, 344, 495, 345, 494,
318, 521, 331, 508, 325, 514, 321, 518, 346, 493, 339, 500, 351, 488, 306, 533, 289, 550, 400, 439,
378, 461, 374, 465, 415, 424, 270, 569, 241, 598, 231, 608, 260, 579, 268, 571, 276, 563, 409, 430,
398, 441, 290, 549, 304, 535, 308, 531, 358, 481, 316, 523, 293, 546, 288, 551, 284, 555, 368, 471,
253, 586, 256, 583, 263, 576, 242, 597, 274, 565, 402, 437, 383, 456, 357, 482, 329, 510, 317, 522,
307, 532, 286, 553, 287, 552, 266, 573, 261, 578, 236, 603, 303, 536, 356, 483, 355, 484, 405, 434,
404, 435, 406, 433, 235, 604, 267, 572, 302, 537, 309, 530, 265, 574, 233, 606, 367, 472, 296, 543,
336, 503, 305, 534, 373, 466, 280, 559, 279, 560, 419, 420, 240, 599, 258, 581, 229, 610
};
// Table 6.3.3.1-4: Mapping from logical index i to sequence number u for preamble formats with L_RA = 139
uint16_t prach_root_sequence_map_abc[138] = {
1 , 138, 2 , 137, 3 , 136, 4 , 135, 5 , 134, 6 , 133, 7 , 132, 8 , 131, 9 , 130, 10, 129,
11, 128, 12, 127, 13, 126, 14, 125, 15, 124, 16, 123, 17, 122, 18, 121, 19, 120, 20, 119,
21, 118, 22, 117, 23, 116, 24, 115, 25, 114, 26, 113, 27, 112, 28, 111, 29, 110, 30, 109,
31, 108, 32, 107, 33, 106, 34, 105, 35, 104, 36, 103, 37, 102, 38, 101, 39, 100, 40, 99 ,
41, 98 , 42, 97 , 43, 96 , 44, 95 , 45, 94 , 46, 93 , 47, 92 , 48, 91 , 49, 90 , 50, 89 ,
51, 88 , 52, 87 , 53, 86 , 54, 85 , 55, 84 , 56, 83 , 57, 82 , 58, 81 , 59, 80 , 60, 79 ,
61, 78 , 62, 77 , 63, 76 , 64, 75 , 65, 74 , 66, 73 , 67, 72 , 68, 71 , 69, 70
};

// Table 6.3.3.2-2: Random access configurations for FR1 and paired spectrum/supplementary uplink
// the column 5, (SFN_nbr is a bitmap where we set bit to '1' in the position of the subframe where the RACH can be sent.
// E.g. in row 4, and column 5 we have set value 512 ('1000000000') which means RACH can be sent at subframe 9.
// E.g. in row 20 and column 5 we have set value 66  ('0001000010') which means RACH can be sent at subframe 1 or 6
int64_t table_6_3_3_2_2_prachConfig_Index [256][9] = {
//format,   format,       x,          y,        SFN_nbr,   star_symb,   slots_sfn,    occ_slot,  duration
{0,          -1,          16,         1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{0,          -1,          16,         1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{0,          -1,          16,         1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{0,          -1,          16,         1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{0,          -1,          8,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{0,          -1,          8,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{0,          -1,          8,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{0,          -1,          8,          1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{0,          -1,          4,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{0,          -1,          4,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{0,          -1,          4,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{0,          -1,          4,          1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{0,          -1,          2,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{0,          -1,          2,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{0,          -1,          2,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{0,          -1,          2,          1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{0,          -1,          1,          0,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{0,          -1,          1,          0,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{0,          -1,          1,          0,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{0,          -1,          1,          0,          66,         0,         -1,         -1,          0},          // (subframe number)           1,6
{0,          -1,          1,          0,          132,        0,         -1,         -1,          0},          // (subframe number)           2,7
{0,          -1,          1,          0,          264,        0,         -1,         -1,          0},          // (subframe number)           3,8
{0,          -1,          1,          0,          146,        0,         -1,         -1,          0},          // (subframe number)           1,4,7
{0,          -1,          1,          0,          292,        0,         -1,         -1,          0},          // (subframe number)           2,5,8
{0,          -1,          1,          0,          584,        0,         -1,         -1,          0},          // (subframe number)           3, 6, 9
{0,          -1,          1,          0,          341,        0,         -1,         -1,          0},          // (subframe number)           0,2,4,6,8
{0,          -1,          1,          0,          682,        0,         -1,         -1,          0},          // (subframe number)           1,3,5,7,9
{0,          -1,          1,          0,          1023,       0,         -1,         -1,          0},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{1,          -1,          16,         1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{1,          -1,          16,         1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{1,          -1,          16,         1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{1,          -1,          16,         1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{1,          -1,          8,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{1,          -1,          8,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{1,          -1,          8,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{1,          -1,          8,          1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{1,          -1,          4,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{1,          -1,          4,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{1,          -1,          4,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{1,          -1,          4,          1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{1,          -1,          2,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{1,          -1,          2,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{1,          -1,          2,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{1,          -1,          2,          1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{1,          -1,          1,          0,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{1,          -1,          1,          0,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{1,          -1,          1,          0,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{1,          -1,          1,          0,          66,         0,         -1,         -1,          0},          // (subframe number)           1,6
{1,          -1,          1,          0,          132,        0,         -1,         -1,          0},          // (subframe number)           2,7
{1,          -1,          1,          0,          264,        0,         -1,         -1,          0},          // (subframe number)           3,8
{1,          -1,          1,          0,          146,        0,         -1,         -1,          0},          // (subframe number)           1,4,7
{1,          -1,          1,          0,          292,        0,         -1,         -1,          0},          // (subframe number)           2,5,8
{1,          -1,          1,          0,          584,        0,         -1,         -1,          0},          // (subframe number)           3,6,9
{2,          -1,          16,         1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{2,          -1,          8,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{2,          -1,          4,          0,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{2,          -1,          2,          0,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{2,          -1,          2,          0,          32,         0,         -1,         -1,          0},          // (subframe number)           5
{2,          -1,          1,          0,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{2,          -1,          1,          0,          32,         0,         -1,         -1,          0},          // (subframe number)           5
{3,          -1,          16,         1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{3,          -1,          16,         1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{3,          -1,          16,         1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{3,          -1,          16,         1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{3,          -1,          8,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{3,          -1,          8,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{3,          -1,          8,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{3,          -1,          4,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{3,          -1,          4,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{3,          -1,          4,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{3,          -1,          4,          1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{3,          -1,          2,          1,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{3,          -1,          2,          1,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{3,          -1,          2,          1,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{3,          -1,          2,          1,          512,        0,         -1,         -1,          0},          // (subframe number)           9
{3,          -1,          1,          0,          2,          0,         -1,         -1,          0},          // (subframe number)           1
{3,          -1,          1,          0,          16,         0,         -1,         -1,          0},          // (subframe number)           4
{3,          -1,          1,          0,          128,        0,         -1,         -1,          0},          // (subframe number)           7
{3,          -1,          1,          0,          66,         0,         -1,         -1,          0},          // (subframe number)           1,6
{3,          -1,          1,          0,          132,        0,         -1,         -1,          0},          // (subframe number)           2,7
{3,          -1,          1,          0,          264,        0,         -1,         -1,          0},          // (subframe number)           3,8
{3,          -1,          1,          0,          146,        0,         -1,         -1,          0},          // (subframe number)           1,4,7
{3,          -1,          1,          0,          292,        0,         -1,         -1,          0},          // (subframe number)           2,5,8
{3,          -1,          1,          0,          584,        0,         -1,         -1,          0},          // (subframe number)           3, 6, 9
{3,          -1,          1,          0,          341,        0,         -1,         -1,          0},          // (subframe number)           0,2,4,6,8
{3,          -1,          1,          0,          682,        0,         -1,         -1,          0},          // (subframe number)           1,3,5,7,9
{3,          -1,          1,          0,          1023,       0,         -1,         -1,          0},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa1,       -1,          16,         0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          16,         1,          16,         0,          2,          6,          2},          // (subframe number)           4
{0xa1,       -1,          8,          0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          8,          1,          16,         0,          2,          6,          2},          // (subframe number)           4
{0xa1,       -1,          4,          0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          4,          1,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          4,          0,          16,         0,          2,          6,          2},          // (subframe number)           4
{0xa1,       -1,          2,          0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          2,          0,          2,          0,          2,          6,          2},          // (subframe number)           1
{0xa1,       -1,          2,          0,          16,         0,          2,          6,          2},          // (subframe number)           4
{0xa1,       -1,          2,          0,          128,        0,          2,          6,          2},          // (subframe number)           7
{0xa1,       -1,          1,          0,          16,         0,          1,          6,          2},          // (subframe number)           4
{0xa1,       -1,          1,          0,          66,         0,          1,          6,          2},          // (subframe number)           1,6
{0xa1,       -1,          1,          0,          528,        0,          1,          6,          2},          // (subframe number)           4,9
{0xa1,       -1,          1,          0,          2,          0,          2,          6,          2},          // (subframe number)           1
{0xa1,       -1,          1,          0,          128,        0,          2,          6,          2},          // (subframe number)           7
{0xa1,       -1,          1,          0,          132,        0,          2,          6,          2},          // (subframe number)           2,7
{0xa1,       -1,          1,          0,          146,        0,          2,          6,          2},          // (subframe number)           1,4,7
{0xa1,       -1,          1,          0,          341,        0,          2,          6,          2},          // (subframe number)           0,2,4,6,8
{0xa1,       -1,          1,          0,          1023,       0,          2,          6,          2},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa1,       -1,          1,          0,          682,        0,          2,          6,          2},          // (subframe number)           1,3,5,7,9
{0xa1,       0xb1,        2,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xa1,       0xb1,        2,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xa1,       0xb1,        1,          0,          16,         0,          1,          7,          2},          // (subframe number)           4
{0xa1,       0xb1,        1,          0,          66,         0,          1,          7,          2},          // (subframe number)           1,6
{0xa1,       0xb1,        1,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xa1,       0xb1,        1,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xa1,       0xb1,        1,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xa1,       0xb1,        1,          0,          146,        0,          2,          7,          2},          // (subframe number)           1,4,7
{0xa1,       0xb1,        1,          0,          341,        0,          2,          7,          2},          // (subframe number)           0,2,4,6,8
{0xa2,       -1,          16,         1,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       -1,          16,         1,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       -1,          8,          1,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       -1,          8,          1,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       -1,          4,          0,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       -1,          4,          0,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       -1,          2,          1,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       -1,          2,          0,          2,          0,          2,          3,          4},          // (subframe number)           1
{0xa2,       -1,          2,          0,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       -1,          2,          0,          128,        0,          2,          3,          4},          // (subframe number)           7
{0xa2,       -1,          1,          0,          16,         0,          1,          3,          4},          // (subframe number)           4
{0xa2,       -1,          1,          0,          66,         0,          1,          3,          4},          // (subframe number)           1,6
{0xa2,       -1,          1,          0,          528,        0,          1,          3,          4},          // (subframe number)           4,9
{0xa2,       -1,          1,          0,          2,          0,          2,          3,          4},          // (subframe number)           1
{0xa2,       -1,          1,          0,          128,        0,          2,          3,          4},          // (subframe number)           7
{0xa2,       -1,          1,          0,          132,        0,          2,          3,          4},          // (subframe number)           2,7
{0xa2,       -1,          1,          0,          146,        0,          2,          3,          4},          // (subframe number)           1,4,7
{0xa2,       -1,          1,          0,          341,        0,          2,          3,          4},          // (subframe number)           0,2,4,6,8
{0xa2,       -1,          1,          0,          1023,       0,          2,          3,          4},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa2,       -1,          1,          0,          682,        0,          2,          3,          4},          // (subframe number)           1,3,5,7,9
{0xa2,       0xb2,        2,          1,          580,        0,          1,          3,          4},          // (subframe number)           2,6,9
{0xa2,       0xb2,        2,          0,          16,         0,          2,          3,          4},          // (subframe number)           4
{0xa2,       0xb2,        1,          0,          16,         0,          1,          3,          4},          // (subframe number)           4
{0xa2,       0xb2,        1,          0,          66,         0,          1,          3,          4},          // (subframe number)           1,6
{0xa2,       0xb2,        1,          0,          528,        0,          1,          3,          4},          // (subframe number)           4,9
{0xa2,       0xb2,        1,          0,          2,          0,          2,          3,          4},          // (subframe number)           1
{0xa2,       0xb2,        1,          0,          128,        0,          2,          3,          4},          // (subframe number)           7
{0xa2,       0xb2,        1,          0,          146,        0,          2,          3,          4},          // (subframe number)           1,4,7
{0xa2,       0xb2,        1,          0,          341,        0,          2,          3,          4},          // (subframe number)           0,2,4,6,8
{0xa2,       0xb2,        1,          0,          1023,       0,          2,          3,          4},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa3,       -1,          16,         1,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       -1,          16,         1,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       -1,          8,          1,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       -1,          8,          1,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       -1,          4,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       -1,          4,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       -1,          2,          1,          580,        0,          2,          2,          6},          // (subframe number)           2,6,9
{0xa3,       -1,          2,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xa3,       -1,          2,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       -1,          2,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xa3,       -1,          1,          0,          16,         0,          1,          2,          6},          // (subframe number)           4
{0xa3,       -1,          1,          0,          66,         0,          1,          2,          6},          // (subframe number)           1,6
{0xa3,       -1,          1,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       -1,          1,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xa3,       -1,          1,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xa3,       -1,          1,          0,          132,        0,          2,          2,          6},          // (subframe number)           2,7
{0xa3,       -1,          1,          0,          146,        0,          2,          2,          6},          // (subframe number)           1,4,7
{0xa3,       -1,          1,          0,          341,        0,          2,          2,          6},          // (subframe number)           0,2,4,6,8
{0xa3,       -1,          1,          0,          1023,       0,          2,          2,          6},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xa3,       -1,          1,          0,          682,        0,          2,          2,          6},          // (subframe number)           1,3,5,7,9
{0xa3,       0xb3,        2,          1,          580,        0,          2,          2,          6},          // (subframe number)           2,6,9
{0xa3,       0xb3,        2,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xa3,       0xb3,        1,          0,          16,         0,          1,          2,          6},          // (subframe number)           4
{0xa3,       0xb3,        1,          0,          66,         0,          1,          2,          6},          // (subframe number)           1,6
{0xa3,       0xb3,        1,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xa3,       0xb3,        1,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xa3,       0xb3,        1,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xa3,       0xb3,        1,          0,          146,        0,          2,          2,          6},          // (subframe number)           1,4,7
{0xa3,       0xb3,        1,          0,          341,        0,          2,          2,          6},          // (subframe number)           0,2,4,6,8
{0xa3,       0xb3,        1,          0,          1023,       0,          2,          2,          6},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xb1,       -1,          16,         0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          16,         1,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xb1,       -1,          8,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          8,          1,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xb1,       -1,          4,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          4,          1,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          4,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xb1,       -1,          2,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          2,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xb1,       -1,          2,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xb1,       -1,          2,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xb1,       -1,          1,          0,          16,         0,          1,          7,          2},          // (subframe number)           4
{0xb1,       -1,          1,          0,          66,         0,          1,          7,          2},          // (subframe number)           1,6
{0xb1,       -1,          1,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xb1,       -1,          1,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xb1,       -1,          1,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xb1,       -1,          1,          0,          132,        0,          2,          7,          2},          // (subframe number)           2,7
{0xb1,       -1,          1,          0,          146,        0,          2,          7,          2},          // (subframe number)           1,4,7
{0xb1,       -1,          1,          0,          341,        0,          2,          7,          2},          // (subframe number)           0,2,4,6,8
{0xb1,       -1,          1,          0,          1023,       0,          2,          7,          2},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xb1,       -1,          1,          0,          682,        0,          2,          7,          2},          // (subframe number)           1,3,5,7,9
{0xb4,       -1,          16,         0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          16,         1,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          8,          0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          8,          1,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          4,          0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          4,          0,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          4,          1,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          2,          0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          2,          0,          2,          0,          2,          1,          12},         // (subframe number)           1
{0xb4,       -1,          2,          0,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          2,          0,          128,        0,          2,          1,          12},         // (subframe number)           7
{0xb4,       -1,          1,          0,          2,          0,          2,          1,          12},         // (subframe number)           1
{0xb4,       -1,          1,          0,          16,         0,          2,          1,          12},         // (subframe number)           4
{0xb4,       -1,          1,          0,          128,        0,          2,          1,          12},         // (subframe number)           7
{0xb4,       -1,          1,          0,          66,         0,          2,          1,          12},         // (subframe number)           1,6
{0xb4,       -1,          1,          0,          132,        0,          2,          1,          12},         // (subframe number)           2,7
{0xb4,       -1,          1,          0,          528,        0,          2,          1,          12},         // (subframe number)           4,9
{0xb4,       -1,          1,          0,          146,        0,          2,          1,          12},         // (subframe number)           1,4,7
{0xb4,       -1,          1,          0,          341,        0,          2,          1,          12},         // (subframe number)           0,2,4,6,8
{0xb4,       -1,          1,          0,          1023,       0,          2,          1,          12},         // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xb4,       -1,          1,          0,          682,        0,          2,          1,          12},         // (subframe number)           1,3,5,7,9
{0xc0,       -1,          8,          1,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xc0,       -1,          4,          1,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xc0,       -1,          4,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xc0,       -1,          2,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xc0,       -1,          2,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xc0,       -1,          2,          0,          16,         0,          2,          7,          2},          // (subframe number)           4
{0xc0,       -1,          2,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xc0,       -1,          1,          0,          16,         0,          1,          7,          2},          // (subframe number)           4
{0xc0,       -1,          1,          0,          66,         0,          1,          7,          2},          // (subframe number)           1,6
{0xc0,       -1,          1,          0,          528,        0,          1,          7,          2},          // (subframe number)           4,9
{0xc0,       -1,          1,          0,          2,          0,          2,          7,          2},          // (subframe number)           1
{0xc0,       -1,          1,          0,          128,        0,          2,          7,          2},          // (subframe number)           7
{0xc0,       -1,          1,          0,          132,        0,          2,          7,          2},          // (subframe number)           2,7
{0xc0,       -1,          1,          0,          146,        0,          2,          7,          2},          // (subframe number)           1,4,7
{0xc0,       -1,          1,          0,          341,        0,          2,          7,          2},          // (subframe number)           0,2,4,6,8
{0xc0,       -1,          1,          0,          1023,       0,          2,          7,          2},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xc0,       -1,          1,          0,          682,        0,          2,          7,          2},          // (subframe number)           1,3,5,7,9
{0xc2,       -1,          16,         1,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xc2,       -1,          16,         1,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xc2,       -1,          8,          1,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xc2,       -1,          8,          1,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xc2,       -1,          4,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xc2,       -1,          4,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xc2,       -1,          2,          1,          580,        0,          2,          2,          6},          // (subframe number)           2,6,9
{0xc2,       -1,          2,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xc2,       -1,          2,          0,          16,         0,          2,          2,          6},          // (subframe number)           4
{0xc2,       -1,          2,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xc2,       -1,          1,          0,          16,         0,          1,          2,          6},          // (subframe number)           4
{0xc2,       -1,          1,          0,          66,         0,          1,          2,          6},          // (subframe number)           1,6
{0xc2,       -1,          1,          0,          528,        0,          1,          2,          6},          // (subframe number)           4,9
{0xc2,       -1,          1,          0,          2,          0,          2,          2,          6},          // (subframe number)           1
{0xc2,       -1,          1,          0,          128,        0,          2,          2,          6},          // (subframe number)           7
{0xc2,       -1,          1,          0,          132,        0,          2,          2,          6},          // (subframe number)           2,7
{0xc2,       -1,          1,          0,          146,        0,          2,          2,          6},          // (subframe number)           1,4,7
{0xc2,       -1,          1,          0,          341,        0,          2,          2,          6},          // (subframe number)           0,2,4,6,8
{0xc2,       -1,          1,          0,          1023,       0,          2,          2,          6},          // (subframe number)           0,1,2,3,4,5,6,7,8,9
{0xc2,       -1,          1,          0,          682,        0,          2,          2,          6}                    // (subframe number)           1,3,5,7,9
};
// Table 6.3.3.2-3: Random access configurations for FR1 and unpaired spectrum
int64_t table_6_3_3_2_3_prachConfig_Index [256][9] = {
//format,     format,      x,         y,     SFN_nbr,   star_symb,   slots_sfn,  occ_slot,  duration
{0,            -1,         16,        1,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{0,            -1,         8,         1,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{0,            -1,         4,         1,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{0,            -1,         2,         0,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{0,            -1,         2,         1,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{0,            -1,         2,         0,         16,          0,        -1,        -1,         0},         // (subrame number 4)
{0,            -1,         2,         1,         16,          0,        -1,        -1,         0},         // (subrame number 4)
{0,            -1,         1,         0,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{0,            -1,         1,         0,         256,         0,        -1,        -1,         0},         // (subrame number 8)
{0,            -1,         1,         0,         128,         0,        -1,        -1,         0},         // (subrame number 7)
{0,            -1,         1,         0,         64,          0,        -1,        -1,         0},         // (subrame number 6)
{0,            -1,         1,         0,         32,          0,        -1,        -1,         0},         // (subrame number 5)
{0,            -1,         1,         0,         16,          0,        -1,        -1,         0},         // (subrame number 4)
{0,            -1,         1,         0,         8,           0,        -1,        -1,         0},         // (subrame number 3)
{0,            -1,         1,         0,         4,           0,        -1,        -1,         0},         // (subrame number 2)
{0,            -1,         1,         0,         66,          0,        -1,        -1,         0},         // (subrame number 1,6)
{0,            -1,         1,         0,         66,          7,        -1,        -1,         0},         // (subrame number 1,6)
{0,            -1,         1,         0,         528,         0,        -1,        -1,         0},         // (subrame number 4,9)
{0,            -1,         1,         0,         264,         0,        -1,        -1,         0},         // (subrame number 3,8)
{0,            -1,         1,         0,         132,         0,        -1,        -1,         0},         // (subrame number 2,7)
{0,            -1,         1,         0,         768,         0,        -1,        -1,         0},         // (subrame number 8,9)
{0,            -1,         1,         0,         784,         0,        -1,        -1,         0},         // (subrame number 4,8,9)
{0,            -1,         1,         0,         536,         0,        -1,        -1,         0},         // (subrame number 3,4,9)
{0,            -1,         1,         0,         896,         0,        -1,        -1,         0},         // (subrame number 7,8,9)
{0,            -1,         1,         0,         792,         0,        -1,        -1,         0},         // (subrame number 3,4,8,9)
{0,            -1,         1,         0,         960,         0,        -1,        -1,         0},         // (subrame number 6,7,8,9)
{0,            -1,         1,         0,         594,         0,        -1,        -1,         0},         // (subrame number 1,4,6,9)
{0,            -1,         1,         0,         682,         0,        -1,        -1,         0},         // (subrame number 1,3,5,7,9)
{1,            -1,         16,        1,         128,         0,        -1,        -1,         0},         // (subrame number 7)
{1,            -1,         8,         1,         128,         0,        -1,        -1,         0},         // (subrame number 7)
{1,            -1,         4,         1,         128,         0,        -1,        -1,         0},         // (subrame number 7)
{1,            -1,         2,         0,         128,         0,        -1,        -1,         0},         // (subrame number 7)
{1,            -1,         2,         1,         128,         0,        -1,        -1,         0},         // (subrame number 7)
{1,            -1,         1,         0,         128,         0,        -1,        -1,         0},         // (subrame number 7)
{2,            -1,         16,        1,         64,          0,        -1,        -1,         0},         // (subrame number 6)
{2,            -1,         8,         1,         64,          0,        -1,        -1,         0},         // (subrame number 6)
{2,            -1,         4,         1,         64,          0,        -1,        -1,         0},         // (subrame number 6)
{2,            -1,         2,         0,         64,          7,        -1,        -1,         0},         // (subrame number 6)
{2,            -1,         2,         1,         64,          7,        -1,        -1,         0},         // (subrame number 6)
{2,            -1,         1,         0,         64,          7,        -1,        -1,         0},         // (subrame number 6)
{3,            -1,         16,        1,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{3,            -1,         8,         1,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{3,            -1,         4,         1,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{3,            -1,         2,         0,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{3,            -1,         2,         1,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{3,            -1,         2,         0,         16,          0,        -1,        -1,         0},         // (subrame number 4)
{3,            -1,         2,         1,         16,          0,        -1,        -1,         0},         // (subrame number 4)
{3,            -1,         1,         0,         512,         0,        -1,        -1,         0},         // (subrame number 9)
{3,            -1,         1,         0,         256,         0,        -1,        -1,         0},         // (subrame number 8)
{3,            -1,         1,         0,         128,         0,        -1,        -1,         0},         // (subrame number 7)
{3,            -1,         1,         0,         64,          0,        -1,        -1,         0},         // (subrame number 6)
{3,            -1,         1,         0,         32,          0,        -1,        -1,         0},         // (subrame number 5)
{3,            -1,         1,         0,         16,          0,        -1,        -1,         0},         // (subrame number 4)
{3,            -1,         1,         0,         8,           0,        -1,        -1,         0},         // (subrame number 3)
{3,            -1,         1,         0,         4,           0,        -1,        -1,         0},         // (subrame number 2)
{3,            -1,         1,         0,         66,          0,        -1,        -1,         0},         // (subrame number 1,6)
{3,            -1,         1,         0,         66,          7,        -1,        -1,         0},         // (subrame number 1,6)
{3,            -1,         1,         0,         528,         0,        -1,        -1,         0},         // (subrame number 4,9)
{3,            -1,         1,         0,         264,         0,        -1,        -1,         0},         // (subrame number 3,8)
{3,            -1,         1,         0,         132,         0,        -1,        -1,         0},         // (subrame number 2,7)
{3,            -1,         1,         0,         768,         0,        -1,        -1,         0},         // (subrame number 8,9)
{3,            -1,         1,         0,         784,         0,        -1,        -1,         0},         // (subrame number 4,8,9)
{3,            -1,         1,         0,         536,         0,        -1,        -1,         0},         // (subrame number 3,4,9)
{3,            -1,         1,         0,         896,         0,        -1,        -1,         0},         // (subrame number 7,8,9)
{3,            -1,         1,         0,         792,         0,        -1,        -1,         0},         // (subrame number 3,4,8,9)
{3,            -1,         1,         0,         594,         0,        -1,        -1,         0},         // (subrame number 1,4,6,9)
{3,            -1,         1,         0,         682,         0,        -1,        -1,         0},         // (subrame number 1,3,5,7,9)
{0xa1,         -1,         16,        1,         512,         0,         2,         6,         2},         // (subrame number 9)
{0xa1,         -1,         8,         1,         512,         0,         2,         6,         2},         // (subrame number 9)
{0xa1,         -1,         4,         1,         512,         0,         1,         6,         2},         // (subrame number 9)
{0xa1,         -1,         2,         1,         512,         0,         1,         6,         2},         // (subrame number 9)
{0xa1,         -1,         2,         1,         528,         7,         1,         3,         2},         // (subrame number 4,9)
{0xa1,         -1,         2,         1,         640,         7,         1,         3,         2},         // (subrame number 7,9)
{0xa1,         -1,         2,         1,         640,         0,         1,         6,         2},         // (subrame number 7,9)
{0xa1,         -1,         2,         1,         768,         0,         2,         6,         2},         // (subrame number 8,9)
{0xa1,         -1,         2,         1,         528,         0,         2,         6,         2},         // (subrame number 4,9)
{0xa1,         -1,         2,         1,         924,         0,         1,         6,         2},         // (subrame number 2,3,4,7,8,9)
{0xa1,         -1,         1,         0,         512,         0,         2,         6,         2},         // (subrame number 9)
{0xa1,         -1,         1,         0,         512,         7,         1,         3,         2},         // (subrame number 9)
{0xa1,         -1,         1,         0,         512,         0,         1,         6,         2},         // (subrame number 9)
{0xa1,         -1,         1,         0,         768,         0,         2,         6,         2},         // (subrame number 8,9)
{0xa1,         -1,         1,         0,         528,         0,         1,         6,         2},         // (subrame number 4,9)
{0xa1,         -1,         1,         0,         640,         7,         1,         3,         2},         // (subrame number 7,9)
{0xa1,         -1,         1,         0,         792,         0,         1,         6,         2},         // (subrame number 3,4,8,9)
{0xa1,         -1,         1,         0,         792,         0,         2,         6,         2},         // (subrame number 3,4,8,9)
{0xa1,         -1,         1,         0,         682,         0,         1,         6,         2},         // (subrame number 1,3,5,7,9)
{0xa1,         -1,         1,         0,         1023,        7,         1,         3,         2},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa2,         -1,         16,        1,         512,         0,         2,         3,         4},         // (subrame number 9)
{0xa2,         -1,         8,         1,         512,         0,         2,         3,         4},         // (subrame number 9)
{0xa2,         -1,         4,         1,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         -1,         2,         1,         640,         0,         1,         3,         4},         // (subrame number 7,9)
{0xa2,         -1,         2,         1,         768,         0,         2,         3,         4},         // (subrame number 8,9)
{0xa2,         -1,         2,         1,         640,         9,         1,         1,         4},         // (subrame number 7,9)
{0xa2,         -1,         2,         1,         528,         9,         1,         1,         4},         // (subrame number 4,9)
{0xa2,         -1,         2,         1,         528,         0,         2,         3,         4},         // (subrame number 4,9)
{0xa2,         -1,         16,        1,         924,         0,         1,         3,         4},         // (subrame number 2,3,4,7,8,9)
{0xa2,         -1,         1,         0,         4,           0,         1,         3,         4},         // (subrame number 2)
{0xa2,         -1,         1,         0,         128,         0,         1,         3,         4},         // (subrame number 7)
{0xa2,         -1,         2,         1,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         -1,         1,         0,         512,         0,         2,         3,         4},         // (subrame number 9)
{0xa2,         -1,         1,         0,         512,         9,         1,         1,         4},         // (subrame number 9)
{0xa2,         -1,         1,         0,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         -1,         1,         0,         132,         0,         1,         3,         4},         // (subrame number 2,7)
{0xa2,         -1,         1,         0,         768,         0,         2,         3,         4},         // (subrame number 8,9)
{0xa2,         -1,         1,         0,         528,         0,         1,         3,         4},         // (subrame number 4,9)
{0xa2,         -1,         1,         0,         640,         9,         1,         1,         4},         // (subrame number 7,9)
{0xa2,         -1,         1,         0,         792,         0,         1,         3,         4},         // (subrame number 3,4,8,9)
{0xa2,         -1,         1,         0,         792,         0,         2,         3,         4},         // (subrame number 3,4,8,9)
{0xa2,         -1,         1,         0,         682,         0,         1,         3,         4},         // (subrame number 1,3,5,7,9)
{0xa2,         -1,         1,         0,         1023,        9,         1,         1,         4},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa3,         -1,         16,        1,         512,         0,         2,         2,         6},         // (subrame number 9)
{0xa3,         -1,         8,         1,         512,         0,         2,         2,         6},         // (subrame number 9)
{0xa3,         -1,         4,         1,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         -1,         2,         1,         528,         7,         1,         1,         6},         // (subrame number 4,9)
{0xa3,         -1,         2,         1,         640,         7,         1,         1,         6},         // (subrame number 7,9)
{0xa3,         -1,         2,         1,         640,         0,         1,         2,         6},         // (subrame number 7,9)
{0xa3,         -1,         2,         1,         528,         0,         2,         2,         6},         // (subrame number 4,9)
{0xa3,         -1,         2,         1,         768,         0,         2,         2,         6},         // (subrame number 8,9)
{0xa3,         -1,         2,         1,         924,         0,         1,         2,         6},         // (subrame number 2,3,4,7,8,9)
{0xa3,         -1,         1,         0,         4,           0,         1,         2,         6},         // (subrame number 2)
{0xa3,         -1,         1,         0,         128,         0,         1,         2,         6},         // (subrame number 7)
{0xa3,         -1,         2,         1,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         -1,         1,         0,         512,         0,         2,         2,         6},         // (subrame number 9)
{0xa3,         -1,         1,         0,         512,         7,         1,         1,         6},         // (subrame number 9)
{0xa3,         -1,         1,         0,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         -1,         1,         0,         132,         0,         1,         2,         6},         // (subrame number 2,7)
{0xa3,         -1,         1,         0,         768,         0,         2,         2,         6},         // (subrame number 8,9)
{0xa3,         -1,         1,         0,         528,         0,         1,         2,         6},         // (subrame number 4,9)
{0xa3,         -1,         1,         0,         640,         7,         1,         1,         6},         // (subrame number 7,9)
{0xa3,         -1,         1,         0,         792,         0,         1,         2,         6},         // (subrame number 3,4,8,9)
{0xa3,         -1,         1,         0,         792,         0,         2,         2,         6},         // (subrame number 3,4,8,9)
{0xa3,         -1,         1,         0,         682,         0,         1,         2,         6},         // (subrame number 1,3,5,7,9)
{0xa3,         -1,         1,         0,         1023,        7,         1,         1,         6},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xb1,         -1,         4,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xb1,         -1,         2,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xb1,         -1,         2,         1,         640,         2,         1,         6,         2},         // (subrame number 7,9)
{0xb1,         -1,         2,         1,         528,         8,         1,         3,         2},         // (subrame number 4,9)
{0xb1,         -1,         2,         1,         528,         2,         2,         6,         2},         // (subrame number 4,9)
{0xb1,         -1,         1,         0,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xb1,         -1,         1,         0,         512,         8,         1,         3,         2},         // (subrame number 9)
{0xb1,         -1,         1,         0,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xb1,         -1,         1,         0,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xb1,         -1,         1,         0,         528,         2,         1,         6,         2},         // (subrame number 4,9)
{0xb1,         -1,         1,         0,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xb1,         -1,         1,         0,         682,         2,         1,         6,         2},         // (subrame number 1,3,5,7,9)
{0xb4,         -1,         16,        1,         512,         0,         2,         1,         12},        // (subrame number 9)
{0xb4,         -1,         8,         1,         512,         0,         2,         1,         12},        // (subrame number 9)
{0xb4,         -1,         4,         1,         512,         2,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         2,         1,         512,         0,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         2,         1,         512,         2,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         2,         1,         640,         2,         1,         1,         12},        // (subrame number 7,9)
{0xb4,         -1,         2,         1,         528,         2,         1,         1,         12},        // (subrame number 4,9)
{0xb4,         -1,         2,         1,         528,         0,         2,         1,         12},        // (subrame number 4,9)
{0xb4,         -1,         2,         1,         768,         0,         2,         1,         12},        // (subrame number 8,9)
{0xb4,         -1,         2,         1,         924,         0,         1,         1,         12},        // (subrame number 2,3,4,7,8,9)
{0xb4,         -1,         1,         0,         2,           0,         1,         1,         12},        // (subrame number 1)
{0xb4,         -1,         1,         0,         4,           0,         1,         1,         12},        // (subrame number 2)
{0xb4,         -1,         1,         0,         16,          0,         1,         1,         12},        // (subrame number 4)
{0xb4,         -1,         1,         0,         128,         0,         1,         1,         12},        // (subrame number 7)
{0xb4,         -1,         1,         0,         512,         0,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         1,         0,         512,         2,         1,         1,         12},        // (subrame number 9)
{0xb4,         -1,         1,         0,         512,         0,         2,         1,         12},        // (subrame number 9)
{0xb4,         -1,         1,         0,         528,         2,         1,         1,         12},        // (subrame number 4,9)
{0xb4,         -1,         1,         0,         640,         2,         1,         1,         12},        // (subrame number 7,9)
{0xb4,         -1,         1,         0,         768,         0,         2,         1,         12},        // (subrame number 8,9)
{0xb4,         -1,         1,         0,         792,         2,         1,         1,         12},        // (subrame number 3,4,8,9)
{0xb4,         -1,         1,         0,         682,         2,         1,         1,         12},        // (subrame number 1,3,5,7,9)
{0xb4,         -1,         1,         0,         1023,        0,         2,         1,         12},        // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xb4,         -1,         1,         0,         1023,        2,         1,         1,         12},        // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xc0,         -1,         16,        1,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xc0,         -1,         8,         1,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xc0,         -1,         4,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xc0,         -1,         2,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xc0,         -1,         2,         1,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xc0,         -1,         2,         1,         640,         2,         1,         6,         2},         // (subrame number 7,9)
{0xc0,         -1,         2,         1,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xc0,         -1,         2,         1,         528,         8,         1,         3,         2},         // (subrame number 4,9)
{0xc0,         -1,         2,         1,         528,         2,         2,         6,         2},         // (subrame number 4,9)
{0xc0,         -1,         2,         1,         924,         2,         1,         6,         2},         // (subrame number 2,3,4,7,8,9)
{0xc0,         -1,         1,         0,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xc0,         -1,         1,         0,         512,         8,         1,         3,         2},         // (subrame number 9)
{0xc0,         -1,         1,         0,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xc0,         -1,         1,         0,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xc0,         -1,         1,         0,         528,         2,         1,         6,         2},         // (subrame number 4,9)
{0xc0,         -1,         1,         0,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xc0,         -1,         1,         0,         792,         2,         1,         6,         2},         // (subrame number 3,4,8,9)
{0xc0,         -1,         1,         0,         792,         2,         2,         6,         2},         // (subrame number 3,4,8,9)
{0xc0,         -1,         1,         0,         682,         2,         1,         6,         2},         // (subrame number 1,3,5,7,9)
{0xc0,         -1,         1,         0,         1023,        8,         1,         3,         2},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xc2,         -1,         16,        1,         512,         2,         2,         2,         6},         // (subrame number 9)
{0xc2,         -1,         8,         1,         512,         2,         2,         2,         6},         // (subrame number 9)
{0xc2,         -1,         4,         1,         512,         2,         1,         2,         6},         // (subrame number 9)
{0xc2,         -1,         2,         1,         512,         2,         1,         2,         6},         // (subrame number 9)
{0xc2,         -1,         2,         1,         768,         2,         2,         2,         6},         // (subrame number 8,9)
{0xc2,         -1,         2,         1,         640,         2,         1,         2,         6},         // (subrame number 7,9)
{0xc2,         -1,         2,         1,         640,         8,         1,         1,         6},         // (subrame number 7,9)
{0xc2,         -1,         2,         1,         528,         8,         1,         1,         6},         // (subrame number 4,9)
{0xc2,         -1,         2,         1,         528,         2,         2,         2,         6},         // (subrame number 4,9)
{0xc2,         -1,         2,         1,         924,         2,         1,         2,         6},         // (subrame number 2,3,4,7,8,9)
{0xc2,         -1,         8,         1,         512,         8,         2,         1,         6},         // (subrame number 9)
{0xc2,         -1,         4,         1,         512,         8,         1,         1,         6},         // (subrame number 9)
{0xc2,         -1,         1,         0,         512,         2,         2,         2,         6},         // (subrame number 9)
{0xc2,         -1,         1,         0,         512,         8,         1,         1,         6},         // (subrame number 9)
{0xc2,         -1,         1,         0,         512,         2,         1,         2,         6},         // (subrame number 9)
{0xc2,         -1,         1,         0,         768,         2,         2,         2,         6},         // (subrame number 8,9)
{0xc2,         -1,         1,         0,         528,         2,         1,         2,         6},         // (subrame number 4,9)
{0xc2,         -1,         1,         0,         640,         8,         1,         1,         6},         // (subrame number 7,9)
{0xc2,         -1,         1,         0,         792,         2,         1,         2,         6},         // (subrame number 3,4,8,9)
{0xc2,         -1,         1,         0,         792,         2,         2,         2,         6},         // (subrame number 3,4,8,9)
{0xc2,         -1,         1,         0,         682,         2,         1,         2,         6},         // (subrame number 1,3,5,7,9)
{0xc2,         -1,         1,         0,         1023,        8,         1,         1,         6},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa1,         0xb1,       2,         1,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xa1,         0xb1,       2,         1,         528,         8,         1,         3,         2},         // (subrame number 4,9)
{0xa1,         0xb1,       2,         1,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xa1,         0xb1,       2,         1,         640,         2,         1,         6,         2},         // (subrame number 7,9)
{0xa1,         0xb1,       2,         1,         528,         2,         2,         6,         2},         // (subrame number 4,9)
{0xa1,         0xb1,       2,         1,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xa1,         0xb1,       1,         0,         512,         2,         2,         6,         2},         // (subrame number 9)
{0xa1,         0xb1,       1,         0,         512,         8,         1,         3,         2},         // (subrame number 9)
{0xa1,         0xb1,       1,         0,         512,         2,         1,         6,         2},         // (subrame number 9)
{0xa1,         0xb1,       1,         0,         768,         2,         2,         6,         2},         // (subrame number 8,9)
{0xa1,         0xb1,       1,         0,         528,         2,         1,         6,         2},         // (subrame number 4,9)
{0xa1,         0xb1,       1,         0,         640,         8,         1,         3,         2},         // (subrame number 7,9)
{0xa1,         0xb1,       1,         0,         792,         2,         2,         6,         2},         // (subrame number 3,4,8,9)
{0xa1,         0xb1,       1,         0,         682,         2,         1,         6,         2},         // (subrame number 1,3,5,7,9)
{0xa1,         0xb1,       1,         0,         1023,        8,         1,         3,         2},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa2,         0xb2,       2,         1,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         0xb2,       2,         1,         528,         6,         1,         2,         4},         // (subrame number 4,9)
{0xa2,         0xb2,       2,         1,         640,         6,         1,         2,         4},         // (subrame number 7,9)
{0xa2,         0xb2,       2,         1,         528,         0,         2,         3,         4},         // (subrame number 4,9)
{0xa2,         0xb2,       2,         1,         768,         0,         2,         3,         4},         // (subrame number 8,9)
{0xa2,         0xb2,       1,         0,         512,         0,         2,         3,         4},         // (subrame number 9)
{0xa2,         0xb2,       1,         0,         512,         6,         1,         2,         4},         // (subrame number 9)
{0xa2,         0xb2,       1,         0,         512,         0,         1,         3,         4},         // (subrame number 9)
{0xa2,         0xb2,       1,         0,         768,         0,         2,         3,         4},         // (subrame number 8,9)
{0xa2,         0xb2,       1,         0,         528,         0,         1,         3,         4},         // (subrame number 4,9)
{0xa2,         0xb2,       1,         0,         640,         6,         1,         2,         4},         // (subrame number 7,9)
{0xa2,         0xb2,       1,         0,         792,         0,         1,         3,         4},         // (subrame number 3,4,8,9)
{0xa2,         0xb2,       1,         0,         792,         0,         2,         3,         4},         // (subrame number 3,4,8,9)
{0xa2,         0xb2,       1,         0,         682,         0,         1,         3,         4},         // (subrame number 1,3,5,7,9)
{0xa2,         0xb2,       1,         0,         1023,        6,         1,         2,         4},         // (subrame number 0,1,2,3,4,5,6,7,8,9)
{0xa3,         0xb3,       2,         1,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         0xb3,       2,         1,         528,         2,         1,         2,         6},         // (subrame number 4,9)
{0xa3,         0xb3,       2,         1,         640,         0,         1,         2,         6},         // (subrame number 7,9)
{0xa3,         0xb3,       2,         1,         640,         2,         1,         2,         6},         // (subrame number 7,9)
{0xa3,         0xb3,       2,         1,         528,         0,         2,         2,         6},         // (subrame number 4,9)
{0xa3,         0xb3,       2,         1,         768,         0,         2,         2,         6},         // (subrame number 8,9)
{0xa3,         0xb3,       1,         0,         512,         0,         2,         2,         6},         // (subrame number 9)
{0xa3,         0xb3,       1,         0,         512,         2,         1,         2,         6},         // (subrame number 9)
{0xa3,         0xb3,       1,         0,         512,         0,         1,         2,         6},         // (subrame number 9)
{0xa3,         0xb3,       1,         0,         768,         0,         2,         2,         6},         // (subrame number 8,9)
{0xa3,         0xb3,       1,         0,         528,         0,         1,         2,         6},         // (subrame number 4,9)
{0xa3,         0xb3,       1,         0,         640,         2,         1,         2,         6},         // (subrame number 7,9)
{0xa3,         0xb3,       1,         0,         792,         0,         2,         2,         6},         // (subrame number 3,4,8,9)
{0xa3,         0xb3,       1,         0,         682,         0,         1,         2,         6},         // (subrame number 1,3,5,7,9)
{0xa3,         0xb3,       1,         0,         1023,        2,         1,         2,         6}          // (subrame number 0,1,2,3,4,5,6,7,8,9)
};
// Table 6.3.3.2-4: Random access configurations for FR2 and unpaired spectrum
int64_t table_6_3_3_2_4_prachConfig_Index [256][10] = {
//format,      format,       x,          y,           y,              SFN_nbr,       star_symb,   slots_sfn,  occ_slot,  duration
{0xa1,          -1,          16,         1,          -1,          567489872400,          0,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          16,         1,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          8,          1,           2,          550293209600,          0,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xa1,          -1,          8,          1,          -1,          567489872400,          0,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          8,          1,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          4,          1,          -1,          567489872400,          0,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          4,          1,          -1,          567489872400,          0,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          4,          1,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          2,          1,          -1,          551911719040,          0,          2,          6,          2},          // (subframe number :7,15,23,31,39)
{0xa1,          -1,          2,          1,          -1,          567489872400,          0,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          2,          1,          -1,          567489872400,          0,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          2,          1,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          549756338176,          7,          1,          3,          2},          // (subframe number :19,39)
{0xa1,          -1,          1,          0,          -1,          168,                   0,          1,          6,          2},          // (subframe number :3,5,7)
{0xa1,          -1,          1,          0,          -1,          567489331200,          7,          1,          3,          2},          // (subframe number :24,29,34,39)
{0xa1,          -1,          1,          0,          -1,          550293209600,          7,          2,          3,          2},          // (subframe number :9,19,29,39)
{0xa1,          -1,          1,          0,          -1,          687195422720,          0,          1,          6,          2},          // (subframe number :17,19,37,39)
{0xa1,          -1,          1,          0,          -1,          550293209600,          0,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xa1,          -1,          1,          0,          -1,          567489872400,          0,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          1,          0,          -1,          567489872400,          7,          1,          3,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          -1,          1,          0,          -1,          10920,                 7,          1,          3,          2},          // (subframe number :3,5,7,9,11,13)
{0xa1,          -1,          1,          0,          -1,          586405642240,          7,          1,          3,          2},          // (subframe number :23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          551911719040,          0,          1,          6,          2},          // (subframe number :7,15,23,31,39)
{0xa1,          -1,          1,          0,          -1,          586405642240,          0,          1,          6,          2},          // (subframe number :23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          965830828032,          7,          2,          3,          2},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xa1,          -1,          1,          0,          -1,          586406201480,          7,          1,          3,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          586406201480,          0,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          -1,          1,          0,          -1,          733007751850,          0,          1,          6,          2},          // (subframe number :1,3,5,7,…,37,39)
{0xa1,          -1,          1,          0,          -1,          1099511627775,         7,          1,          3,          2},          // (subframe number :0,1,2,…,39)
{0xa2,          -1,          16,         1,          -1,          567489872400,          0,          2,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          16,         1,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          8,          1,          -1,          567489872400,          0,          2,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          8,          1,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          8,          1,           2,          550293209600,          0,          2,          3,          4},          // (subframe number :9,19,29,39)
{0xa2,          -1,          4,          1,          -1,          567489872400,          0,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          4,          1,          -1,          567489872400,          0,          2,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          4,          1,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          2,          1,          -1,          551911719040,          0,          2,          3,          4},          // (subframe number :7,15,23,31,39)
{0xa2,          -1,          2,          1,          -1,          567489872400,          0,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          2,          1,          -1,          567489872400,          0,          2,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          2,          1,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          549756338176,          5,          1,          2,          4},          // (subframe number :19,39)
{0xa2,          -1,          1,          0,          -1,          168,                   0,          1,          3,          4},          // (subframe number :3,5,7)
{0xa2,          -1,          1,          0,          -1,          567489331200,          5,          1,          2,          4},          // (subframe number :24,29,34,39)
{0xa2,          -1,          1,          0,          -1,          550293209600,          5,          2,          2,          4},          // (subframe number :9,19,29,39)
{0xa2,          -1,          1,          0,          -1,          687195422720,          0,          1,          3,          4},          // (subframe number :17,19,37,39)
{0xa2,          -1,          1,          0,          -1,          550293209600,          0,          2,          3,          4},          // (subframe number :9, 19, 29, 39)
{0xa2,          -1,          1,          0,          -1,          551911719040,          0,          1,          3,          4},          // (subframe number :7,15,23,31,39)
{0xa2,          -1,          1,          0,          -1,          586405642240,          5,          1,          2,          4},          // (subframe number :23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          586405642240,          0,          1,          3,          4},          // (subframe number :23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          10920,                 5,          1,          2,          4},          // (subframe number :3,5,7,9,11,13)
{0xa2,          -1,          1,          0,          -1,          10920,                 0,          1,          3,          4},          // (subframe number :3,5,7,9,11,13)
{0xa2,          -1,          1,          0,          -1,          567489872400,          5,          1,          2,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          1,          0,          -1,          567489872400,          0,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          -1,          1,          0,          -1,          965830828032,          5,          2,          2,          4},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xa2,          -1,          1,          0,          -1,          586406201480,          5,          1,          2,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          586406201480,          0,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          -1,          1,          0,          -1,          733007751850,          0,          1,          3,          4},          // (subframe number :1,3,5,7,…,37,39)
{0xa2,          -1,          1,          0,          -1,          1099511627775,         5,          1,          2,          4},          // (subframe number :0,1,2,…,39)
{0xa3,          -1,          16,         1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          16,         1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          8,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          8,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          8,          1,           2,          550293209600,          0,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xa3,          -1,          4,          1,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          4,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          4,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          2,          1,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          2,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          2,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          549756338176,          7,          1,          1,          6},          // (subframe number :19,39)
{0xa3,          -1,          1,          0,          -1,          168,                   0,          1,          2,          6},          // (subframe number :3,5,7)
{0xa3,          -1,          1,          0,          -1,          10752,                 2,          1,          2,          6},          // (subframe number :9,11,13)
{0xa3,          -1,          1,          0,          -1,          567489331200,          7,          1,          1,          6},          // (subframe number :24,29,34,39)
{0xa3,          -1,          1,          0,          -1,          550293209600,          7,          2,          1,          6},          // (subframe number :9,19,29,39)
{0xa3,          -1,          1,          0,          -1,          687195422720,          0,          1,          2,          6},          // (subframe number :17,19,37,39)
{0xa3,          -1,          1,          0,          -1,          550293209600,          0,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xa3,          -1,          1,          0,          -1,          551911719040,          0,          1,          2,          6},          // (subframe number :7,15,23,31,39)
{0xa3,          -1,          1,          0,          -1,          586405642240,          7,          1,          1,          6},          // (subframe number :23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          586405642240,          0,          1,          2,          6},          // (subframe number :23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          10920,                 0,          1,          2,          6},          // (subframe number :3,5,7,9,11,13)
{0xa3,          -1,          1,          0,          -1,          10920,                 7,          1,          1,          6},          // (subframe number :3,5,7,9,11,13)
{0xa3,          -1,          1,          0,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          1,          0,          -1,          567489872400,          7,          1,          1,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          -1,          1,          0,          -1,          965830828032,          7,          2,          1,          6},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xa3,          -1,          1,          0,          -1,          586406201480,          7,          1,          1,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          -1,          1,          0,          -1,          733007751850,          0,          1,          2,          6},          // (subframe number :1,3,5,7,…,37,39)
{0xa3,          -1,          1,          0,          -1,          1099511627775,         7,          1,          1,          6},          // (subframe number :0,1,2,…,39)
{0xb1,          -1,          16,         1,          -1,          567489872400,          2,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          8,          1,          -1,          567489872400,          2,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          8,          1,           2,          550293209600,          2,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xb1,          -1,          4,          1,          -1,          567489872400,          2,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          2,          1,          -1,          567489872400,          2,          2,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          2,          1,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          549756338176,          8,          1,          3,          2},          // (subframe number :19,39)
{0xb1,          -1,          1,          0,          -1,          168,                   2,          1,          6,          2},          // (subframe number :3,5,7)
{0xb1,          -1,          1,          0,          -1,          567489331200,          8,          1,          3,          2},          // (subframe number :24,29,34,39)
{0xb1,          -1,          1,          0,          -1,          550293209600,          8,          2,          3,          2},          // (subframe number :9,19,29,39)
{0xb1,          -1,          1,          0,          -1,          687195422720,          2,          1,          6,          2},          // (subframe number :17,19,37,39)
{0xb1,          -1,          1,          0,          -1,          550293209600,          2,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xb1,          -1,          1,          0,          -1,          551911719040,          2,          1,          6,          2},          // (subframe number :7,15,23,31,39)
{0xb1,          -1,          1,          0,          -1,          586405642240,          8,          1,          3,          2},          // (subframe number :23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          586405642240,          2,          1,          6,          2},          // (subframe number :23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          10920,                 8,          1,          3,          2},          // (subframe number :3,5,7,9,11,13)
{0xb1,          -1,          1,          0,          -1,          567489872400,          8,          1,          3,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          1,          0,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xb1,          -1,          1,          0,          -1,          586406201480,          8,          1,          3,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          965830828032,          8,          2,          3,          2},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xb1,          -1,          1,          0,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb1,          -1,          1,          0,          -1,          733007751850,          2,          1,          6,          2},          // (subframe number :1,3,5,7,…,37,39)
{0xb1,          -1,          1,          0,          -1,          1099511627775,         8,          1,          3,          2},          // (subframe number :0,1,2,…,39)
{0xb4,          -1,          16,         1,           2,          567489872400,          0,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          16,         1,           2,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          8,          1,           2,          567489872400,          0,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          8,          1,           2,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          8,          1,           2,          550293209600,          0,          2,          1,          12},         // (subframe number :9,19,29,39)
{0xb4,          -1,          4,          1,          -1,          567489872400,          0,          1,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          4,          1,          -1,          567489872400,          0,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          4,          1,           2,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          2,          1,          -1,          551911719040,          2,          2,          1,          12},         // (subframe number :7,15,23,31,39)
{0xb4,          -1,          2,          1,          -1,          567489872400,          0,          1,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          2,          1,          -1,          567489872400,          0,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          2,          1,          -1,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          549756338176,          2,          2,          1,          12},         // (subframe number :19, 39)
{0xb4,          -1,          1,          0,          -1,          687195422720,          0,          1,          1,          12},         // (subframe number :17, 19, 37, 39)
{0xb4,          -1,          1,          0,          -1,          567489331200,          2,          1,          1,          12},         // (subframe number :24,29,34,39)
{0xb4,          -1,          1,          0,          -1,          550293209600,          2,          2,          1,          12},         // (subframe number :9,19,29,39)
{0xb4,          -1,          1,          0,          -1,          550293209600,          0,          2,          1,          12},         // (subframe number :9,19,29,39)
{0xb4,          -1,          1,          0,          -1,          551911719040,          0,          1,          1,          12},         // (subframe number :7,15,23,31,39)
{0xb4,          -1,          1,          0,          -1,          551911719040,          0,          2,          1,          12},         // (subframe number :7,15,23,31,39)
{0xb4,          -1,          1,          0,          -1,          586405642240,          0,          1,          1,          12},         // (subframe number :23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          586405642240,          2,          2,          1,          12},         // (subframe number :23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          698880,                0,          1,          1,          12},         // (subframe number :9,11,13,15,17,19)
{0xb4,          -1,          1,          0,          -1,          10920,                 2,          1,          1,          12},         // (subframe number :3,5,7,9,11,13)
{0xb4,          -1,          1,          0,          -1,          567489872400,          0,          1,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          1,          0,          -1,          567489872400,          2,          2,          1,          12},         // (subframe number :4,9,14,19,24,29,34,39)
{0xb4,          -1,          1,          0,          -1,          965830828032,          2,          2,          1,          12},         // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xb4,          -1,          1,          0,          -1,          586406201480,          0,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          586406201480,          2,          1,          1,          12},         // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xb4,          -1,          1,          0,          -1,          44739240,              2,          1,          1,          12},         // (subframe number :3, 5, 7, …, 23,25)
{0xb4,          -1,          1,          0,          -1,          44739240,              0,          2,          1,          12},         // (subframe number :3, 5, 7, …, 23,25)
{0xb4,          -1,          1,          0,          -1,          733007751850,          0,          1,          1,          12},         // (subframe number :1,3,5,7,…,37,39)
{0xb4,          -1,          1,          0,          -1,          1099511627775,         2,          1,          1,          12},         // (subframe number :0, 1, 2,…, 39)
{0xc0,          -1,          16,         1,          -1,          567489872400,          0,          2,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          16,         1,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          8,          1,          -1,          567489872400,          0,          1,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          8,          1,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          8,          1,           2,          550293209600,          0,          2,          7,          2},          // (subframe number :9,19,29,39)
{0xc0,          -1,          4,          1,          -1,          567489872400,          0,          1,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          4,          1,          -1,          567489872400,          0,          2,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          4,          1,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          2,          1,          -1,          551911719040,          0,          2,          7,          2},          // (subframe number :7,15,23,31,39)
{0xc0,          -1,          2,          1,          -1,          567489872400,          0,          1,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          2,          1,          -1,          567489872400,          0,          2,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          2,          1,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          549756338176,          8,          1,          3,          2},          // (subframe number :19,39)
{0xc0,          -1,          1,          0,          -1,          168,                   0,          1,          7,          2},          // (subframe number :3,5,7)
{0xc0,          -1,          1,          0,          -1,          567489331200,          8,          1,          3,          2},          // (subframe number :24,29,34,39)
{0xc0,          -1,          1,          0,          -1,          550293209600,          8,          2,          3,          2},          // (subframe number :9,19,29,39)
{0xc0,          -1,          1,          0,          -1,          687195422720,          0,          1,          7,          2},          // (subframe number :17,19,37,39)
{0xc0,          -1,          1,          0,          -1,          550293209600,          0,          2,          7,          2},          // (subframe number :9,19,29,39)
{0xc0,          -1,          1,          0,          -1,          586405642240,          8,          1,          3,          2},          // (subframe number :23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          551911719040,          0,          1,          7,          2},          // (subframe number :7,15,23,31,39)
{0xc0,          -1,          1,          0,          -1,          586405642240,          0,          1,          7,          2},          // (subframe number :23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          10920,                 8,          1,          3,          2},          // (subframe number :3,5,7,9,11,13)
{0xc0,          -1,          1,          0,          -1,          567489872400,          8,          1,          3,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          1,          0,          -1,          567489872400,          0,          1,          7,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc0,          -1,          1,          0,          -1,          965830828032,          8,          2,          3,          2},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xc0,          -1,          1,          0,          -1,          586406201480,          8,          1,          3,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          586406201480,          0,          1,          7,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc0,          -1,          1,          0,          -1,          733007751850,          0,          1,          7,          2},          // (subframe number :1,3,5,7,…,37,39)
{0xc0,          -1,          1,          0,          -1,          1099511627775,         8,          1,          3,          2},          // (subframe number :0,1,2,…,39)
{0xc2,          -1,          16,         1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          16,         1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          8,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          8,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          8,          1,           2,          550293209600,          0,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xc2,          -1,          4,          1,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          4,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          4,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          2,          1,          -1,          551911719040,          2,          2,          2,          6},          // (subframe number :7,15,23,31,39)
{0xc2,          -1,          2,          1,          -1,          567489872400,          0,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          2,          1,          -1,          567489872400,          0,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          2,          1,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          549756338176,          2,          1,          2,          6},          // (subframe number :19,39)
{0xc2,          -1,          1,          0,          -1,          168,                   0,          1,          2,          6},          // (subframe number :3,5,7)
{0xc2,          -1,          1,          0,          -1,          567489331200,          7,          1,          1,          6},          // (subframe number :24,29,34,39)
{0xc2,          -1,          1,          0,          -1,          550293209600,          7,          2,          1,          6},          // (subframe number :9,19,29,39)
{0xc2,          -1,          1,          0,          -1,          687195422720,          0,          1,          2,          6},          // (subframe number :17,19,37,39)
{0xc2,          -1,          1,          0,          -1,          550293209600,          2,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xc2,          -1,          1,          0,          -1,          551911719040,          2,          1,          2,          6},          // (subframe number :7,15,23,31,39)
{0xc2,          -1,          1,          0,          -1,          10920,                 7,          1,          1,          6},          // (subframe number :3,5,7,9,11,13)
{0xc2,          -1,          1,          0,          -1,          586405642240,          7,          2,          1,          6},          // (subframe number :23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          586405642240,          0,          1,          2,          6},          // (subframe number :23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          567489872400,          7,          2,          1,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          1,          0,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xc2,          -1,          1,          0,          -1,          965830828032,          7,          2,          1,          6},          // (subframe number :13,14,15, 29,30,31,37,38,39)
{0xc2,          -1,          1,          0,          -1,          586406201480,          7,          1,          1,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          586406201480,          0,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xc2,          -1,          1,          0,          -1,          733007751850,          0,          1,          2,          6},          // (subframe number :1,3,5,7,…,37,39)
{0xc2,          -1,          1,          0,          -1,          1099511627775,         7,          1,          1,          6},          // (subframe number :0,1,2,…,39)
{0xa1,          0xb1,        16,         1,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        16,         1,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          0xb1,        8,          1,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        8,          1,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          0xb1,        4,          1,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        4,          1,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          0xb1,        2,          1,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        1,          0,          -1,          549756338176,          8,          1,          3,          2},          // (subframe number :19,39)
{0xa1,          0xb1,        1,          0,          -1,          550293209600,          8,          1,          3,          2},          // (subframe number :9,19,29,39)
{0xa1,          0xb1,        1,          0,          -1,          687195422720,          2,          1,          6,          2},          // (subframe number :17,19,37,39)
{0xa1,          0xb1,        1,          0,          -1,          550293209600,          2,          2,          6,          2},          // (subframe number :9,19,29,39)
{0xa1,          0xb1,        1,          0,          -1,          586405642240,          8,          1,          3,          2},          // (subframe number :23,27,31,35,39)
{0xa1,          0xb1,        1,          0,          -1,          551911719040,          2,          1,          6,          2},          // (subframe number :7,15,23,31,39)
{0xa1,          0xb1,        1,          0,          -1,          586405642240,          2,          1,          6,          2},          // (subframe number :23,27,31,35,39)
{0xa1,          0xb1,        1,          0,          -1,          567489872400,          8,          1,          3,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        1,          0,          -1,          567489872400,          2,          1,          6,          2},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa1,          0xb1,        1,          0,          -1,          586406201480,          2,          1,          6,          2},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa1,          0xb1,        1,          0,          -1,          733007751850,          2,          1,          6,          2},          // (subframe number :1,3,5,7,…,37,39)
{0xa2,          0xb2,        16,         1,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        16,         1,          -1,          586406201480,          2,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          0xb2,        8,          1,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        8,          1,          -1,          586406201480,          2,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          0xb2,        4,          1,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        4,          1,          -1,          586406201480,          2,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          0xb2,        2,          1,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        1,          0,          -1,          549756338176,          6,          1,          2,          4},          // (subframe number :19,39)
{0xa2,          0xb2,        1,          0,          -1,          550293209600,          6,          1,          2,          4},          // (subframe number :9,19,29,39)
{0xa2,          0xb2,        1,          0,          -1,          687195422720,          2,          1,          3,          4},          // (subframe number :17,19,37,39)
{0xa2,          0xb2,        1,          0,          -1,          550293209600,          2,          2,          3,          4},          // (subframe number :9,19,29,39)
{0xa2,          0xb2,        1,          0,          -1,          586405642240,          6,          1,          2,          4},          // (subframe number :23,27,31,35,39)
{0xa2,          0xb2,        1,          0,          -1,          551911719040,          2,          1,          3,          4},          // (subframe number :7,15,23,31,39)
{0xa2,          0xb2,        1,          0,          -1,          586405642240,          2,          1,          3,          4},          // (subframe number :23,27,31,35,39)
{0xa2,          0xb2,        1,          0,          -1,          567489872400,          6,          1,          2,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        1,          0,          -1,          567489872400,          2,          1,          3,          4},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa2,          0xb2,        1,          0,          -1,          586406201480,          2,          1,          3,          4},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa2,          0xb2,        1,          0,          -1,          733007751850,          2,          1,          3,          4},          // (subframe number :1,3,5,7,…,37,39)
{0xa3,          0xb3,        16,         1,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        16,         1,          -1,          586406201480,          2,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          0xb3,        8,          1,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        8,          1,          -1,          586406201480,          2,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          0xb3,        4,          1,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        4,          1,          -1,          586406201480,          2,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          0xb3,        2,          1,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        1,          0,          -1,          549756338176,          2,          1,          2,          6},          // (subframe number :19,39)
{0xa3,          0xb3,        1,          0,          -1,          550293209600,          2,          1,          2,          6},          // (subframe number :9,19,29,39)
{0xa3,          0xb3,        1,          0,          -1,          687195422720,          2,          1,          2,          6},          // (subframe number :17,19,37,39)
{0xa3,          0xb3,        1,          0,          -1,          550293209600,          2,          2,          2,          6},          // (subframe number :9,19,29,39)
{0xa3,          0xb3,        1,          0,          -1,          551911719040,          2,          1,          2,          6},          // (subframe number :7,15,23,31,39)
{0xa3,          0xb3,        1,          0,          -1,          586405642240,          2,          1,          2,          6},          // (subframe number :23,27,31,35,39)
{0xa3,          0xb3,        1,          0,          -1,          586405642240,          2,          2,          2,          6},          // (subframe number :23,27,31,35,39)
{0xa3,          0xb3,        1,          0,          -1,          567489872400,          2,          1,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        1,          0,          -1,          567489872400,          2,          2,          2,          6},          // (subframe number :4,9,14,19,24,29,34,39)
{0xa3,          0xb3,        1,          0,          -1,          586406201480,          2,          1,          2,          6},          // (subframe number :3,7,11,15,19,23,27,31,35,39)
{0xa3,          0xb3,        1,          0,          -1,          733007751850,          2,          1,          2,          6}           // (subframe number :1,3,5,7,…,37,39)
};
