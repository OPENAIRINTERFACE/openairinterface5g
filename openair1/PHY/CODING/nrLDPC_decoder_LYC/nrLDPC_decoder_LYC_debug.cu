#include <stdio.h>
#include <unistd.h>
#include <cuda_runtime.h>
#include <cuda.h>
#include "PHY/CODING/nrLDPC_decoder_LYC/nrLDPC_types.h"
#include "PHY/CODING/nrLDPC_decoder_LYC/nrLDPC_defs.h"

#define MAX_ITERATION 5
#define CW	1

#define cudaCheck(ans) { cudaAssert((ans), __FILE__, __LINE__); }
inline void cudaAssert(cudaError_t code, const char *file, int line)
{
   if (code != cudaSuccess){
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      exit(code);
   }
}
// row and col element count



typedef struct{
  char x;
  char y;
  short value;
} h_element;

__device__ __constant__ char h_element_count1_bg1[46] = {
	19, 19, 19, 19, 3,  8,  9,  7, 10,  9,
	7,  8,  7,  6,  7,  7,  6,  6,  6,  6,  
	6,  6,  5,  5,  6,  5,  5,  4,  5,  5,  
	5,  5,  5,  5,  5,  5,  5,  4,  5,  5,  
	4,  5,  4,  5,  5,  4};
__device__ __constant__ char h_element_count2_bg1[68] = {
	30, 28,  7, 11,  9,  4,  8, 12,  8,  7, 
	12, 10, 12, 11, 10,  7, 10, 10, 13,  7,  
	8,  11, 12,  5,  6,  6,  1,  1,  1,  1,  
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,  
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,  
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,  
	1,   1,  1,  1,  1,  1,  1,  1};
__device__ __constant__ char h_element_count1_bg2[42] = { 
	8, 10,  8, 10,  4,  6,  6,  6,  4,  5, 
	5,  5,  4,  5,  5,  4,  5,  5,  4,  4,
	4,  4,  3,  4,  4,  3,  5,  3,  4,  3,
	5,  3,  4,  4,  4,  4,  4,  3,  4,  4, 
	4,  4};
__device__ __constant__ char h_element_count2_bg2[52] = {
	22, 23, 10,  5,  5, 14,  7, 13,  6,  8, 
	9,  16,  9, 12,  1,  1,  1,  1,  1,  1,
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,   1,  1,  1,  1,  1,  1,  1,  1,  1, 
	1,   1};

// BG
__device__ __constant__ h_element dev_h_base1_1[316]={
  { .x= 0, .y= 0, .value=307 },  { .x= 0, .y= 1, .value= 19 },  { .x= 0, .y= 2, .value= 50 },  { .x= 0, .y= 3, .value=369 },  { .x= 0, .y= 5, .value=181 },  { .x= 0, .y= 6, .value=216 },  { .x= 0, .y= 9, .value=317 },  { .x= 0, .y=10, .value=288 },  { .x= 0, .y=11, .value=109 },  { .x= 0, .y=12, .value= 17 },
  { .x= 0, .y=13, .value=357 },  { .x= 0, .y=15, .value=215 },  { .x= 0, .y=16, .value=106 },  { .x= 0, .y=18, .value=242 },  { .x= 0, .y=19, .value=180 },  { .x= 0, .y=20, .value=330 },  { .x= 0, .y=21, .value=346 },  { .x= 0, .y=22, .value=  1 },  { .x= 0, .y=23, .value=  0 },  { .x= 1, .y= 0, .value= 76 },
  { .x= 1, .y= 2, .value= 76 },  { .x= 1, .y= 3, .value= 73 },  { .x= 1, .y= 4, .value=288 },  { .x= 1, .y= 5, .value=144 },  { .x= 1, .y= 7, .value=331 },  { .x= 1, .y= 8, .value=331 },  { .x= 1, .y= 9, .value=178 },  { .x= 1, .y=11, .value=295 },  { .x= 1, .y=12, .value=342 },  { .x= 1, .y=14, .value=217 },
  { .x= 1, .y=15, .value= 99 },  { .x= 1, .y=16, .value=354 },  { .x= 1, .y=17, .value=114 },  { .x= 1, .y=19, .value=331 },  { .x= 1, .y=21, .value=112 },  { .x= 1, .y=22, .value=  0 },  { .x= 1, .y=23, .value=  0 },  { .x= 1, .y=24, .value=  0 },  { .x= 2, .y= 0, .value=205 },  { .x= 2, .y= 1, .value=250 },
  { .x= 2, .y= 2, .value=328 },  { .x= 2, .y= 4, .value=332 },  { .x= 2, .y= 5, .value=256 },  { .x= 2, .y= 6, .value=161 },  { .x= 2, .y= 7, .value=267 },  { .x= 2, .y= 8, .value=160 },  { .x= 2, .y= 9, .value= 63 },  { .x= 2, .y=10, .value=129 },  { .x= 2, .y=13, .value=200 },  { .x= 2, .y=14, .value= 88 },
  { .x= 2, .y=15, .value= 53 },  { .x= 2, .y=17, .value=131 },  { .x= 2, .y=18, .value=240 },  { .x= 2, .y=19, .value=205 },  { .x= 2, .y=20, .value= 13 },  { .x= 2, .y=24, .value=  0 },  { .x= 2, .y=25, .value=  0 },  { .x= 3, .y= 0, .value=276 },  { .x= 3, .y= 1, .value= 87 },  { .x= 3, .y= 3, .value=  0 },
  { .x= 3, .y= 4, .value=275 },  { .x= 3, .y= 6, .value=199 },  { .x= 3, .y= 7, .value=153 },  { .x= 3, .y= 8, .value= 56 },  { .x= 3, .y=10, .value=132 },  { .x= 3, .y=11, .value=305 },  { .x= 3, .y=12, .value=231 },  { .x= 3, .y=13, .value=341 },  { .x= 3, .y=14, .value=212 },  { .x= 3, .y=16, .value=304 },
  { .x= 3, .y=17, .value=300 },  { .x= 3, .y=18, .value=271 },  { .x= 3, .y=20, .value= 39 },  { .x= 3, .y=21, .value=357 },  { .x= 3, .y=22, .value=  1 },  { .x= 3, .y=25, .value=  0 },  { .x= 4, .y= 0, .value=332 },  { .x= 4, .y= 1, .value=181 },  { .x= 4, .y=26, .value=  0 },  { .x= 5, .y= 0, .value=195 },
  { .x= 5, .y= 1, .value= 14 },  { .x= 5, .y= 3, .value=115 },  { .x= 5, .y=12, .value=166 },  { .x= 5, .y=16, .value=241 },  { .x= 5, .y=21, .value= 51 },  { .x= 5, .y=22, .value=157 },  { .x= 5, .y=27, .value=  0 },  { .x= 6, .y= 0, .value=278 },  { .x= 6, .y= 6, .value=257 },  { .x= 6, .y=10, .value=  1 },
  { .x= 6, .y=11, .value=351 },  { .x= 6, .y=13, .value= 92 },  { .x= 6, .y=17, .value=253 },  { .x= 6, .y=18, .value= 18 },  { .x= 6, .y=20, .value=225 },  { .x= 6, .y=28, .value=  0 },  { .x= 7, .y= 0, .value=  9 },  { .x= 7, .y= 1, .value= 62 },  { .x= 7, .y= 4, .value=316 },  { .x= 7, .y= 7, .value=333 },
  { .x= 7, .y= 8, .value=290 },  { .x= 7, .y=14, .value=114 },  { .x= 7, .y=29, .value=  0 },  { .x= 8, .y= 0, .value=307 },  { .x= 8, .y= 1, .value=179 },  { .x= 8, .y= 3, .value=165 },  { .x= 8, .y=12, .value= 18 },  { .x= 8, .y=16, .value= 39 },  { .x= 8, .y=19, .value=224 },  { .x= 8, .y=21, .value=368 },
  { .x= 8, .y=22, .value= 67 },  { .x= 8, .y=24, .value=170 },  { .x= 8, .y=30, .value=  0 },  { .x= 9, .y= 0, .value=366 },  { .x= 9, .y= 1, .value=232 },  { .x= 9, .y=10, .value=321 },  { .x= 9, .y=11, .value=133 },  { .x= 9, .y=13, .value= 57 },  { .x= 9, .y=17, .value=303 },  { .x= 9, .y=18, .value= 63 },
  { .x= 9, .y=20, .value= 82 },  { .x= 9, .y=31, .value=  0 },  { .x=10, .y= 1, .value=101 },  { .x=10, .y= 2, .value=339 },  { .x=10, .y= 4, .value=274 },  { .x=10, .y= 7, .value=111 },  { .x=10, .y= 8, .value=383 },  { .x=10, .y=14, .value=354 },  { .x=10, .y=32, .value=  0 },  { .x=11, .y= 0, .value= 48 },
  { .x=11, .y= 1, .value=102 },  { .x=11, .y=12, .value=  8 },  { .x=11, .y=16, .value= 47 },  { .x=11, .y=21, .value=188 },  { .x=11, .y=22, .value=334 },  { .x=11, .y=23, .value=115 },  { .x=11, .y=33, .value=  0 },  { .x=12, .y= 0, .value= 77 },  { .x=12, .y= 1, .value=186 },  { .x=12, .y=10, .value=174 },
  { .x=12, .y=11, .value=232 },  { .x=12, .y=13, .value= 50 },  { .x=12, .y=18, .value= 74 },  { .x=12, .y=34, .value=  0 },  { .x=13, .y= 0, .value=313 },  { .x=13, .y= 3, .value=177 },  { .x=13, .y= 7, .value=266 },  { .x=13, .y=20, .value=115 },  { .x=13, .y=23, .value=370 },  { .x=13, .y=35, .value=  0 },
  { .x=14, .y= 0, .value=142 },  { .x=14, .y=12, .value=248 },  { .x=14, .y=15, .value=137 },  { .x=14, .y=16, .value= 89 },  { .x=14, .y=17, .value=347 },  { .x=14, .y=21, .value= 12 },  { .x=14, .y=36, .value=  0 },  { .x=15, .y= 0, .value=241 },  { .x=15, .y= 1, .value=  2 },  { .x=15, .y=10, .value=210 },
  { .x=15, .y=13, .value=318 },  { .x=15, .y=18, .value= 55 },  { .x=15, .y=25, .value=269 },  { .x=15, .y=37, .value=  0 },  { .x=16, .y= 1, .value= 13 },  { .x=16, .y= 3, .value=338 },  { .x=16, .y=11, .value= 57 },  { .x=16, .y=20, .value=289 },  { .x=16, .y=22, .value= 57 },  { .x=16, .y=38, .value=  0 },
  { .x=17, .y= 0, .value=260 },  { .x=17, .y=14, .value=303 },  { .x=17, .y=16, .value= 81 },  { .x=17, .y=17, .value=358 },  { .x=17, .y=21, .value=375 },  { .x=17, .y=39, .value=  0 },  { .x=18, .y= 1, .value=130 },  { .x=18, .y=12, .value=163 },  { .x=18, .y=13, .value=280 },  { .x=18, .y=18, .value=132 },
  { .x=18, .y=19, .value=  4 },  { .x=18, .y=40, .value=  0 },  { .x=19, .y= 0, .value=145 },  { .x=19, .y= 1, .value=213 },  { .x=19, .y= 7, .value=344 },  { .x=19, .y= 8, .value=242 },  { .x=19, .y=10, .value=197 },  { .x=19, .y=41, .value=  0 },  { .x=20, .y= 0, .value=187 },  { .x=20, .y= 3, .value=206 },
  { .x=20, .y= 9, .value=264 },  { .x=20, .y=11, .value=341 },  { .x=20, .y=22, .value= 59 },  { .x=20, .y=42, .value=  0 },  { .x=21, .y= 1, .value=205 },  { .x=21, .y= 5, .value=102 },  { .x=21, .y=16, .value=328 },  { .x=21, .y=20, .value=213 },  { .x=21, .y=21, .value= 97 },  { .x=21, .y=43, .value=  0 },
  { .x=22, .y= 0, .value= 30 },  { .x=22, .y=12, .value= 11 },  { .x=22, .y=13, .value=233 },  { .x=22, .y=17, .value= 22 },  { .x=22, .y=44, .value=  0 },  { .x=23, .y= 1, .value= 24 },  { .x=23, .y= 2, .value= 89 },  { .x=23, .y=10, .value= 61 },  { .x=23, .y=18, .value= 27 },  { .x=23, .y=45, .value=  0 },
  { .x=24, .y= 0, .value=298 },  { .x=24, .y= 3, .value=158 },  { .x=24, .y= 4, .value=235 },  { .x=24, .y=11, .value=339 },  { .x=24, .y=22, .value=234 },  { .x=24, .y=46, .value=  0 },  { .x=25, .y= 1, .value= 72 },  { .x=25, .y= 6, .value= 17 },  { .x=25, .y= 7, .value=383 },  { .x=25, .y=14, .value=312 },
  { .x=25, .y=47, .value=  0 },  { .x=26, .y= 0, .value= 71 },  { .x=26, .y= 2, .value= 81 },  { .x=26, .y= 4, .value= 76 },  { .x=26, .y=15, .value=136 },  { .x=26, .y=48, .value=  0 },  { .x=27, .y= 1, .value=194 },  { .x=27, .y= 6, .value=194 },  { .x=27, .y= 8, .value=101 },  { .x=27, .y=49, .value=  0 },
  { .x=28, .y= 0, .value=222 },  { .x=28, .y= 4, .value= 19 },  { .x=28, .y=19, .value=244 },  { .x=28, .y=21, .value=274 },  { .x=28, .y=50, .value=  0 },  { .x=29, .y= 1, .value=252 },  { .x=29, .y=14, .value=  5 },  { .x=29, .y=18, .value=147 },  { .x=29, .y=25, .value= 78 },  { .x=29, .y=51, .value=  0 },
  { .x=30, .y= 0, .value=159 },  { .x=30, .y=10, .value=229 },  { .x=30, .y=13, .value=260 },  { .x=30, .y=24, .value= 90 },  { .x=30, .y=52, .value=  0 },  { .x=31, .y= 1, .value=100 },  { .x=31, .y= 7, .value=215 },  { .x=31, .y=22, .value=258 },  { .x=31, .y=25, .value=256 },  { .x=31, .y=53, .value=  0 },
  { .x=32, .y= 0, .value=102 },  { .x=32, .y=12, .value=201 },  { .x=32, .y=14, .value=175 },  { .x=32, .y=24, .value=287 },  { .x=32, .y=54, .value=  0 },  { .x=33, .y= 1, .value=323 },  { .x=33, .y= 2, .value=  8 },  { .x=33, .y=11, .value=361 },  { .x=33, .y=21, .value=105 },  { .x=33, .y=55, .value=  0 },
  { .x=34, .y= 0, .value=230 },  { .x=34, .y= 7, .value=148 },  { .x=34, .y=15, .value=202 },  { .x=34, .y=17, .value=312 },  { .x=34, .y=56, .value=  0 },  { .x=35, .y= 1, .value=320 },  { .x=35, .y= 6, .value=335 },  { .x=35, .y=12, .value=  2 },  { .x=35, .y=22, .value=266 },  { .x=35, .y=57, .value=  0 },
  { .x=36, .y= 0, .value=210 },  { .x=36, .y=14, .value=313 },  { .x=36, .y=15, .value=297 },  { .x=36, .y=18, .value= 21 },  { .x=36, .y=58, .value=  0 },  { .x=37, .y= 1, .value=269 },  { .x=37, .y=13, .value= 82 },  { .x=37, .y=23, .value=115 },  { .x=37, .y=59, .value=  0 },  { .x=38, .y= 0, .value=185 },
  { .x=38, .y= 9, .value=177 },  { .x=38, .y=10, .value=289 },  { .x=38, .y=12, .value=214 },  { .x=38, .y=60, .value=  0 },  { .x=39, .y= 1, .value=258 },  { .x=39, .y= 3, .value= 93 },  { .x=39, .y= 7, .value=346 },  { .x=39, .y=19, .value=297 },  { .x=39, .y=61, .value=  0 },  { .x=40, .y= 0, .value=175 },
  { .x=40, .y= 8, .value= 37 },  { .x=40, .y=17, .value=312 },  { .x=40, .y=62, .value=  0 },  { .x=41, .y= 1, .value= 52 },  { .x=41, .y= 3, .value=314 },  { .x=41, .y= 9, .value=139 },  { .x=41, .y=18, .value=288 },  { .x=41, .y=63, .value=  0 },  { .x=42, .y= 0, .value=113 },  { .x=42, .y= 4, .value= 14 },
  { .x=42, .y=24, .value=218 },  { .x=42, .y=64, .value=  0 },  { .x=43, .y= 1, .value=113 },  { .x=43, .y=16, .value=132 },  { .x=43, .y=18, .value=114 },  { .x=43, .y=25, .value=168 },  { .x=43, .y=65, .value=  0 },  { .x=44, .y= 0, .value= 80 },  { .x=44, .y= 7, .value= 78 },  { .x=44, .y= 9, .value=163 },
  { .x=44, .y=22, .value=274 },  { .x=44, .y=66, .value=  0 },  { .x=45, .y= 1, .value=135 },  { .x=45, .y= 6, .value=149 },  { .x=45, .y=10, .value= 15 },  { .x=45, .y=67, .value=  0 } };
__device__ __constant__ h_element* dev_h_compact1[46]={
  &(dev_h_base1_1[  0]),  &(dev_h_base1_1[ 19]),  &(dev_h_base1_1[ 38]),  &(dev_h_base1_1[ 57]),  &(dev_h_base1_1[ 76]),  &(dev_h_base1_1[ 79]),  &(dev_h_base1_1[ 87]),  &(dev_h_base1_1[ 96]),  &(dev_h_base1_1[103]),  &(dev_h_base1_1[113]),
  &(dev_h_base1_1[122]),  &(dev_h_base1_1[129]),  &(dev_h_base1_1[137]),  &(dev_h_base1_1[144]),  &(dev_h_base1_1[150]),  &(dev_h_base1_1[157]),  &(dev_h_base1_1[164]),  &(dev_h_base1_1[170]),  &(dev_h_base1_1[176]),  &(dev_h_base1_1[182]),
  &(dev_h_base1_1[188]),  &(dev_h_base1_1[194]),  &(dev_h_base1_1[200]),  &(dev_h_base1_1[205]),  &(dev_h_base1_1[210]),  &(dev_h_base1_1[216]),  &(dev_h_base1_1[221]),  &(dev_h_base1_1[226]),  &(dev_h_base1_1[230]),  &(dev_h_base1_1[235]),
  &(dev_h_base1_1[240]),  &(dev_h_base1_1[245]),  &(dev_h_base1_1[250]),  &(dev_h_base1_1[255]),  &(dev_h_base1_1[260]),  &(dev_h_base1_1[265]),  &(dev_h_base1_1[270]),  &(dev_h_base1_1[275]),  &(dev_h_base1_1[279]),  &(dev_h_base1_1[284]),
  &(dev_h_base1_1[289]),  &(dev_h_base1_1[293]),  &(dev_h_base1_1[298]),  &(dev_h_base1_1[302]),  &(dev_h_base1_1[307]),  &(dev_h_base1_1[312]) };
__device__ __constant__ h_element dev_h_base2_1[316]={
  { .x= 0, .y= 0, .value=307 },  { .x= 1, .y= 0, .value= 76 },  { .x= 2, .y= 0, .value=205 },  { .x= 3, .y= 0, .value=276 },  { .x= 4, .y= 0, .value=332 },  { .x= 5, .y= 0, .value=195 },  { .x= 6, .y= 0, .value=278 },  { .x= 7, .y= 0, .value=  9 },  { .x= 8, .y= 0, .value=307 },  { .x= 9, .y= 0, .value=366 },
  { .x=11, .y= 0, .value= 48 },  { .x=12, .y= 0, .value= 77 },  { .x=13, .y= 0, .value=313 },  { .x=14, .y= 0, .value=142 },  { .x=15, .y= 0, .value=241 },  { .x=17, .y= 0, .value=260 },  { .x=19, .y= 0, .value=145 },  { .x=20, .y= 0, .value=187 },  { .x=22, .y= 0, .value= 30 },  { .x=24, .y= 0, .value=298 },
  { .x=26, .y= 0, .value= 71 },  { .x=28, .y= 0, .value=222 },  { .x=30, .y= 0, .value=159 },  { .x=32, .y= 0, .value=102 },  { .x=34, .y= 0, .value=230 },  { .x=36, .y= 0, .value=210 },  { .x=38, .y= 0, .value=185 },  { .x=40, .y= 0, .value=175 },  { .x=42, .y= 0, .value=113 },  { .x=44, .y= 0, .value= 80 },
  { .x= 0, .y= 1, .value= 19 },  { .x= 2, .y= 1, .value=250 },  { .x= 3, .y= 1, .value= 87 },  { .x= 4, .y= 1, .value=181 },  { .x= 5, .y= 1, .value= 14 },  { .x= 7, .y= 1, .value= 62 },  { .x= 8, .y= 1, .value=179 },  { .x= 9, .y= 1, .value=232 },  { .x=10, .y= 1, .value=101 },  { .x=11, .y= 1, .value=102 },
  { .x=12, .y= 1, .value=186 },  { .x=15, .y= 1, .value=  2 },  { .x=16, .y= 1, .value= 13 },  { .x=18, .y= 1, .value=130 },  { .x=19, .y= 1, .value=213 },  { .x=21, .y= 1, .value=205 },  { .x=23, .y= 1, .value= 24 },  { .x=25, .y= 1, .value= 72 },  { .x=27, .y= 1, .value=194 },  { .x=29, .y= 1, .value=252 },
  { .x=31, .y= 1, .value=100 },  { .x=33, .y= 1, .value=323 },  { .x=35, .y= 1, .value=320 },  { .x=37, .y= 1, .value=269 },  { .x=39, .y= 1, .value=258 },  { .x=41, .y= 1, .value= 52 },  { .x=43, .y= 1, .value=113 },  { .x=45, .y= 1, .value=135 },  { .x= 0, .y= 2, .value= 50 },  { .x= 1, .y= 2, .value= 76 },
  { .x= 2, .y= 2, .value=328 },  { .x=10, .y= 2, .value=339 },  { .x=23, .y= 2, .value= 89 },  { .x=26, .y= 2, .value= 81 },  { .x=33, .y= 2, .value=  8 },  { .x= 0, .y= 3, .value=369 },  { .x= 1, .y= 3, .value= 73 },  { .x= 3, .y= 3, .value=  0 },  { .x= 5, .y= 3, .value=115 },  { .x= 8, .y= 3, .value=165 },
  { .x=13, .y= 3, .value=177 },  { .x=16, .y= 3, .value=338 },  { .x=20, .y= 3, .value=206 },  { .x=24, .y= 3, .value=158 },  { .x=39, .y= 3, .value= 93 },  { .x=41, .y= 3, .value=314 },  { .x= 1, .y= 4, .value=288 },  { .x= 2, .y= 4, .value=332 },  { .x= 3, .y= 4, .value=275 },  { .x= 7, .y= 4, .value=316 },
  { .x=10, .y= 4, .value=274 },  { .x=24, .y= 4, .value=235 },  { .x=26, .y= 4, .value= 76 },  { .x=28, .y= 4, .value= 19 },  { .x=42, .y= 4, .value= 14 },  { .x= 0, .y= 5, .value=181 },  { .x= 1, .y= 5, .value=144 },  { .x= 2, .y= 5, .value=256 },  { .x=21, .y= 5, .value=102 },  { .x= 0, .y= 6, .value=216 },
  { .x= 2, .y= 6, .value=161 },  { .x= 3, .y= 6, .value=199 },  { .x= 6, .y= 6, .value=257 },  { .x=25, .y= 6, .value= 17 },  { .x=27, .y= 6, .value=194 },  { .x=35, .y= 6, .value=335 },  { .x=45, .y= 6, .value=149 },  { .x= 1, .y= 7, .value=331 },  { .x= 2, .y= 7, .value=267 },  { .x= 3, .y= 7, .value=153 },
  { .x= 7, .y= 7, .value=333 },  { .x=10, .y= 7, .value=111 },  { .x=13, .y= 7, .value=266 },  { .x=19, .y= 7, .value=344 },  { .x=25, .y= 7, .value=383 },  { .x=31, .y= 7, .value=215 },  { .x=34, .y= 7, .value=148 },  { .x=39, .y= 7, .value=346 },  { .x=44, .y= 7, .value= 78 },  { .x= 1, .y= 8, .value=331 },
  { .x= 2, .y= 8, .value=160 },  { .x= 3, .y= 8, .value= 56 },  { .x= 7, .y= 8, .value=290 },  { .x=10, .y= 8, .value=383 },  { .x=19, .y= 8, .value=242 },  { .x=27, .y= 8, .value=101 },  { .x=40, .y= 8, .value= 37 },  { .x= 0, .y= 9, .value=317 },  { .x= 1, .y= 9, .value=178 },  { .x= 2, .y= 9, .value= 63 },
  { .x=20, .y= 9, .value=264 },  { .x=38, .y= 9, .value=177 },  { .x=41, .y= 9, .value=139 },  { .x=44, .y= 9, .value=163 },  { .x= 0, .y=10, .value=288 },  { .x= 2, .y=10, .value=129 },  { .x= 3, .y=10, .value=132 },  { .x= 6, .y=10, .value=  1 },  { .x= 9, .y=10, .value=321 },  { .x=12, .y=10, .value=174 },
  { .x=15, .y=10, .value=210 },  { .x=19, .y=10, .value=197 },  { .x=23, .y=10, .value= 61 },  { .x=30, .y=10, .value=229 },  { .x=38, .y=10, .value=289 },  { .x=45, .y=10, .value= 15 },  { .x= 0, .y=11, .value=109 },  { .x= 1, .y=11, .value=295 },  { .x= 3, .y=11, .value=305 },  { .x= 6, .y=11, .value=351 },
  { .x= 9, .y=11, .value=133 },  { .x=12, .y=11, .value=232 },  { .x=16, .y=11, .value= 57 },  { .x=20, .y=11, .value=341 },  { .x=24, .y=11, .value=339 },  { .x=33, .y=11, .value=361 },  { .x= 0, .y=12, .value= 17 },  { .x= 1, .y=12, .value=342 },  { .x= 3, .y=12, .value=231 },  { .x= 5, .y=12, .value=166 },
  { .x= 8, .y=12, .value= 18 },  { .x=11, .y=12, .value=  8 },  { .x=14, .y=12, .value=248 },  { .x=18, .y=12, .value=163 },  { .x=22, .y=12, .value= 11 },  { .x=32, .y=12, .value=201 },  { .x=35, .y=12, .value=  2 },  { .x=38, .y=12, .value=214 },  { .x= 0, .y=13, .value=357 },  { .x= 2, .y=13, .value=200 },
  { .x= 3, .y=13, .value=341 },  { .x= 6, .y=13, .value= 92 },  { .x= 9, .y=13, .value= 57 },  { .x=12, .y=13, .value= 50 },  { .x=15, .y=13, .value=318 },  { .x=18, .y=13, .value=280 },  { .x=22, .y=13, .value=233 },  { .x=30, .y=13, .value=260 },  { .x=37, .y=13, .value= 82 },  { .x= 1, .y=14, .value=217 },
  { .x= 2, .y=14, .value= 88 },  { .x= 3, .y=14, .value=212 },  { .x= 7, .y=14, .value=114 },  { .x=10, .y=14, .value=354 },  { .x=17, .y=14, .value=303 },  { .x=25, .y=14, .value=312 },  { .x=29, .y=14, .value=  5 },  { .x=32, .y=14, .value=175 },  { .x=36, .y=14, .value=313 },  { .x= 0, .y=15, .value=215 },
  { .x= 1, .y=15, .value= 99 },  { .x= 2, .y=15, .value= 53 },  { .x=14, .y=15, .value=137 },  { .x=26, .y=15, .value=136 },  { .x=34, .y=15, .value=202 },  { .x=36, .y=15, .value=297 },  { .x= 0, .y=16, .value=106 },  { .x= 1, .y=16, .value=354 },  { .x= 3, .y=16, .value=304 },  { .x= 5, .y=16, .value=241 },
  { .x= 8, .y=16, .value= 39 },  { .x=11, .y=16, .value= 47 },  { .x=14, .y=16, .value= 89 },  { .x=17, .y=16, .value= 81 },  { .x=21, .y=16, .value=328 },  { .x=43, .y=16, .value=132 },  { .x= 1, .y=17, .value=114 },  { .x= 2, .y=17, .value=131 },  { .x= 3, .y=17, .value=300 },  { .x= 6, .y=17, .value=253 },
  { .x= 9, .y=17, .value=303 },  { .x=14, .y=17, .value=347 },  { .x=17, .y=17, .value=358 },  { .x=22, .y=17, .value= 22 },  { .x=34, .y=17, .value=312 },  { .x=40, .y=17, .value=312 },  { .x= 0, .y=18, .value=242 },  { .x= 2, .y=18, .value=240 },  { .x= 3, .y=18, .value=271 },  { .x= 6, .y=18, .value= 18 },
  { .x= 9, .y=18, .value= 63 },  { .x=12, .y=18, .value= 74 },  { .x=15, .y=18, .value= 55 },  { .x=18, .y=18, .value=132 },  { .x=23, .y=18, .value= 27 },  { .x=29, .y=18, .value=147 },  { .x=36, .y=18, .value= 21 },  { .x=41, .y=18, .value=288 },  { .x=43, .y=18, .value=114 },  { .x= 0, .y=19, .value=180 },
  { .x= 1, .y=19, .value=331 },  { .x= 2, .y=19, .value=205 },  { .x= 8, .y=19, .value=224 },  { .x=18, .y=19, .value=  4 },  { .x=28, .y=19, .value=244 },  { .x=39, .y=19, .value=297 },  { .x= 0, .y=20, .value=330 },  { .x= 2, .y=20, .value= 13 },  { .x= 3, .y=20, .value= 39 },  { .x= 6, .y=20, .value=225 },
  { .x= 9, .y=20, .value= 82 },  { .x=13, .y=20, .value=115 },  { .x=16, .y=20, .value=289 },  { .x=21, .y=20, .value=213 },  { .x= 0, .y=21, .value=346 },  { .x= 1, .y=21, .value=112 },  { .x= 3, .y=21, .value=357 },  { .x= 5, .y=21, .value= 51 },  { .x= 8, .y=21, .value=368 },  { .x=11, .y=21, .value=188 },
  { .x=14, .y=21, .value= 12 },  { .x=17, .y=21, .value=375 },  { .x=21, .y=21, .value= 97 },  { .x=28, .y=21, .value=274 },  { .x=33, .y=21, .value=105 },  { .x= 0, .y=22, .value=  1 },  { .x= 1, .y=22, .value=  0 },  { .x= 3, .y=22, .value=  1 },  { .x= 5, .y=22, .value=157 },  { .x= 8, .y=22, .value= 67 },
  { .x=11, .y=22, .value=334 },  { .x=16, .y=22, .value= 57 },  { .x=20, .y=22, .value= 59 },  { .x=24, .y=22, .value=234 },  { .x=31, .y=22, .value=258 },  { .x=35, .y=22, .value=266 },  { .x=44, .y=22, .value=274 },  { .x= 0, .y=23, .value=  0 },  { .x= 1, .y=23, .value=  0 },  { .x=11, .y=23, .value=115 },
  { .x=13, .y=23, .value=370 },  { .x=37, .y=23, .value=115 },  { .x= 1, .y=24, .value=  0 },  { .x= 2, .y=24, .value=  0 },  { .x= 8, .y=24, .value=170 },  { .x=30, .y=24, .value= 90 },  { .x=32, .y=24, .value=287 },  { .x=42, .y=24, .value=218 },  { .x= 2, .y=25, .value=  0 },  { .x= 3, .y=25, .value=  0 },
  { .x=15, .y=25, .value=269 },  { .x=29, .y=25, .value= 78 },  { .x=31, .y=25, .value=256 },  { .x=43, .y=25, .value=168 },  { .x= 4, .y=26, .value=  0 },  { .x= 5, .y=27, .value=  0 },  { .x= 6, .y=28, .value=  0 },  { .x= 7, .y=29, .value=  0 },  { .x= 8, .y=30, .value=  0 },  { .x= 9, .y=31, .value=  0 },
  { .x=10, .y=32, .value=  0 },  { .x=11, .y=33, .value=  0 },  { .x=12, .y=34, .value=  0 },  { .x=13, .y=35, .value=  0 },  { .x=14, .y=36, .value=  0 },  { .x=15, .y=37, .value=  0 },  { .x=16, .y=38, .value=  0 },  { .x=17, .y=39, .value=  0 },  { .x=18, .y=40, .value=  0 },  { .x=19, .y=41, .value=  0 },
  { .x=20, .y=42, .value=  0 },  { .x=21, .y=43, .value=  0 },  { .x=22, .y=44, .value=  0 },  { .x=23, .y=45, .value=  0 },  { .x=24, .y=46, .value=  0 },  { .x=25, .y=47, .value=  0 },  { .x=26, .y=48, .value=  0 },  { .x=27, .y=49, .value=  0 },  { .x=28, .y=50, .value=  0 },  { .x=29, .y=51, .value=  0 },
  { .x=30, .y=52, .value=  0 },  { .x=31, .y=53, .value=  0 },  { .x=32, .y=54, .value=  0 },  { .x=33, .y=55, .value=  0 },  { .x=34, .y=56, .value=  0 },  { .x=35, .y=57, .value=  0 },  { .x=36, .y=58, .value=  0 },  { .x=37, .y=59, .value=  0 },  { .x=38, .y=60, .value=  0 },  { .x=39, .y=61, .value=  0 },
  { .x=40, .y=62, .value=  0 },  { .x=41, .y=63, .value=  0 },  { .x=42, .y=64, .value=  0 },  { .x=43, .y=65, .value=  0 },  { .x=44, .y=66, .value=  0 },  { .x=45, .y=67, .value=  0 } };
__device__ __constant__ h_element* dev_h_compact2[68]={
  &(dev_h_base2_1[  0]),  &(dev_h_base2_1[ 30]),  &(dev_h_base2_1[ 58]),  &(dev_h_base2_1[ 65]),  &(dev_h_base2_1[ 76]),  &(dev_h_base2_1[ 85]),  &(dev_h_base2_1[ 89]),  &(dev_h_base2_1[ 97]),  &(dev_h_base2_1[109]),  &(dev_h_base2_1[117]),
  &(dev_h_base2_1[124]),  &(dev_h_base2_1[136]),  &(dev_h_base2_1[146]),  &(dev_h_base2_1[158]),  &(dev_h_base2_1[169]),  &(dev_h_base2_1[179]),  &(dev_h_base2_1[186]),  &(dev_h_base2_1[196]),  &(dev_h_base2_1[206]),  &(dev_h_base2_1[219]),
  &(dev_h_base2_1[226]),  &(dev_h_base2_1[234]),  &(dev_h_base2_1[245]),  &(dev_h_base2_1[257]),  &(dev_h_base2_1[262]),  &(dev_h_base2_1[268]),  &(dev_h_base2_1[274]),  &(dev_h_base2_1[275]),  &(dev_h_base2_1[276]),  &(dev_h_base2_1[277]),
  &(dev_h_base2_1[278]),  &(dev_h_base2_1[279]),  &(dev_h_base2_1[280]),  &(dev_h_base2_1[281]),  &(dev_h_base2_1[282]),  &(dev_h_base2_1[283]),  &(dev_h_base2_1[284]),  &(dev_h_base2_1[285]),  &(dev_h_base2_1[286]),  &(dev_h_base2_1[287]),
  &(dev_h_base2_1[288]),  &(dev_h_base2_1[289]),  &(dev_h_base2_1[290]),  &(dev_h_base2_1[291]),  &(dev_h_base2_1[292]),  &(dev_h_base2_1[293]),  &(dev_h_base2_1[294]),  &(dev_h_base2_1[295]),  &(dev_h_base2_1[296]),  &(dev_h_base2_1[297]),
  &(dev_h_base2_1[298]),  &(dev_h_base2_1[299]),  &(dev_h_base2_1[300]),  &(dev_h_base2_1[301]),  &(dev_h_base2_1[302]),  &(dev_h_base2_1[303]),  &(dev_h_base2_1[304]),  &(dev_h_base2_1[305]),  &(dev_h_base2_1[306]),  &(dev_h_base2_1[307]),
  &(dev_h_base2_1[308]),  &(dev_h_base2_1[309]),  &(dev_h_base2_1[310]),  &(dev_h_base2_1[311]),  &(dev_h_base2_1[312]),  &(dev_h_base2_1[313]),  &(dev_h_base2_1[314]),  &(dev_h_base2_1[315]) };




__device__ void cnp_1st(int row, char* dev_llr, char* dev_buf, int blk, int thd)
{
	int subrow = threadIdx.x, subcol = threadIdx.x, Zc = blockDim.x;
	
	for(int i = 0; i < row; i++)
	{
		int cnt = h_element_count1_bg1[i];
		int min1 = INT32_MAX, min2 = INT32_MAX;
		int sign = 0, tsign = 1, idx = 0;
		int irow = subrow + Zc*i;
		
		for(int j = 0; j < cnt; j++)
		{
			h_element tmp = dev_h_compact1[i][j];
			int icol = tmp.y * Zc + (subcol+tmp.value)%Zc;
			
			int Q = dev_llr[icol];
			int Q_abs = (Q>0)? Q:-Q;
			char sq = (Q<0);
			tsign = tsign*(1-sq*2);
			sign |= (sq << j);
			
			if(threadIdx.x == thd && i == blk){
				printf("dev_llr[%d](Q): %d, total_sign: %d, sign: %d\n", icol, dev_llr[icol], tsign, sign);
			}
			
			
			if(Q_abs < min1){
				min2 = min1;
				min1 = Q_abs;
				idx = j;
			}else if(Q_abs < min2){
				min2 = Q_abs;
			}
		}
		
		if(threadIdx.x == thd && i == blk){
			printf("sign: %d, idx: %d, min1: %d, min2: %d\n", sign, idx, min1, min2);
		}
		
		
		for(int j = 0; j < cnt; j++)
		{
			char sq = 1 - 2 * ((sign >> j) & 0x01);
			int temp = tsign * sq * ((j != idx)? min1 : min2);
			h_element tmp = dev_h_compact1[i][j];
			int addr = irow + tmp.y * row * Zc;
			dev_buf[addr] = temp;
			
			if(threadIdx.x == thd && i == blk){
				printf("temp: %d, addr: %d\n", temp, addr);
			}
		}
	}
	__syncthreads();
}

__device__ void bnp(int row, int col, char* dev_llr, char* dev_const_llr, char* dev_buf, int blk, int thd)
{
	int subrow = threadIdx.x, subcol = threadIdx.x, Zc = blockDim.x;
	for(int i = 0; i < col; i++)
	{
		int cnt = h_element_count2_bg1[i];
		int icol = i * Zc + subcol;
		int sum = dev_const_llr[icol];
		
		for(int j = 0; j < cnt; j++)
		{
			h_element tmp = dev_h_compact2[i][j];
			int irow = tmp.x * Zc + (subrow + Zc - tmp.value)%Zc;
			int addr = i * row * Zc + irow;
			sum = sum + dev_buf[addr];
			
			if(threadIdx.x == thd && i == blk){
				printf("dev_buf[%d]: %d, sum: %d, x: %d, value: %d, irow: %d\n", addr, dev_buf[addr], sum, tmp.x, tmp.value, irow);
			}
			
		}
		if(sum > SCHAR_MAX)	sum = SCHAR_MAX;
		if(sum < SCHAR_MIN)	sum = SCHAR_MIN;
		dev_llr[icol] = sum;
	}
	__syncthreads();
}


__device__ void cnp(int row, char* dev_llr, char* dev_buf, int blk, int thd)
{
	int subrow = threadIdx.x, subcol = threadIdx.x, Zc = blockDim.x;
	
	for(int i = 0; i < row; i++)
	{
		int cnt = h_element_count1_bg1[i];
		int min1 = INT32_MAX, min2 = INT32_MAX;
		int sign = 0, tsign = 1, idx = 0;
		int irow = subrow + Zc*i;
		
		for(int j = 0; j < cnt; j++)
		{
			h_element tmp = dev_h_compact1[i][j];
			int icol = tmp.y * Zc + (subcol+tmp.value)%Zc;
			
			int Q_last = dev_buf[tmp.y * row * Zc + irow];
			
			int Q = dev_llr[icol] - Q_last;
			int Q_abs = (Q>0)? Q:-Q;
			char sq = (Q<0);
			tsign = tsign*(1-sq*2);
			sign |= (sq << j);
			
			if(threadIdx.x == thd && i == blk){
				printf("dev_llr[%d](Q): %d, total_sign: %d, sign: %d, last Q: %d, last_pos: %d\n", icol, dev_llr[icol], tsign, sign, Q_last, tmp.y * row * Zc + irow);
			}
			
			
			if(Q_abs < min1){
				min2 = min1;
				min1 = Q_abs;
				idx = j;
			}else if(Q_abs < min2){
				min2 = Q_abs;
			}
		}
		
		if(threadIdx.x == thd && i == blk){
			printf("sign: %d, idx: %d, min1: %d, min2: %d\n", sign, idx, min1, min2);
		}
		
		
		for(int j = 0; j < cnt; j++)
		{
			char sq = 1 - 2 * ((sign >> j) & 0x01);
			int temp = tsign * sq * ((j != idx)? min1 : min2);
			h_element tmp = dev_h_compact1[i][j];
			int addr = irow + tmp.y * row * Zc;
			dev_buf[addr] = temp;
			
			
			if(threadIdx.x == thd && i == blk){
				printf("temp: %d, addr: %d\n", temp, addr);
			}
		}
	}
	__syncthreads();
}



__global__ void ldpc_decoder_gpu(int BG, char *dev_llr, char *dev_const_llr, char *dev_buf, int blk, int thd)
{
	int iteration = 5;
	int row = 46, col = 68;
	if(BG == 2){
		row = 42;
		col = 52;
	}
	//extern __shared__ sh_llr[];
	//for()
		
	for(int i = 0; i < iteration; i++){
		if(i == 0){
			cnp_1st(row, dev_llr, dev_buf, blk, thd);
		}else{
			cnp(row, dev_llr, dev_buf, blk, thd);
		}
		
		bnp(row, col, dev_llr, dev_const_llr, dev_buf, blk, thd);
	}
	
}


__global__ void pack_decoded_bit(char *dev, unsigned char *host)
{
	__shared__ unsigned char tmp[256];
	int tid = blockIdx.x*128 + threadIdx.x;
	int btid = threadIdx.x;
	tmp[btid] = 0;
	
	if(dev[tid] < 0){
		tmp[btid] = 1 << (7-(btid&7));
	}
	__syncthreads();
	
	if(threadIdx.x < 16){
		host[blockIdx.x*16+threadIdx.x] = 0;
		for(int i = 0; i < 8; i++){
			host[blockIdx.x*16+threadIdx.x] += tmp[threadIdx.x*8+i];
		}
	}
}


//void nrLDPC_decoder_LYC(int BG, int row, int col, int Zc, int block_length)
extern "C"
int32_t nrLDPC_decoder_LYC(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out, int block_length)
{
	// alloc mem
	unsigned char *decision = (unsigned char*)p_out;

    uint16_t Zc          = p_decParams->Z;
    uint8_t  BG         = p_decParams->BG;
    uint8_t  numMaxIter = p_decParams->numMaxIter;
    e_nrLDPC_outMode outMode = p_decParams->outMode;
	
	uint8_t row,col;
	if(BG == 1){
		row = 46;
		col = 68;
	}
	else{
		row = 42;
		col = 52;
	}
		// gpu mem size
	int memsize_llr = col * Zc * sizeof(char);
	int memsize_buf = col * Zc * row * sizeof(char);
	
	// gpu 
	char *dev_llr;
	char *dev_const_llr;
	char *dev_buf;
	unsigned char *dev_tmp;
	
	// gpu alloc
	int p;
	cudaCheck( cudaMallocPitch((void**)&dev_llr, (size_t*)&p, memsize_llr, 1) );
	cudaCheck( cudaMallocPitch((void**)&dev_const_llr, (size_t*)&p, memsize_llr, 1) );
	cudaCheck( cudaMallocPitch((void**)&dev_buf, (size_t*)&p, memsize_buf, 1) );
	cudaCheck( cudaMallocPitch((void**)&dev_tmp, (size_t*)&p, memsize_llr, 1) );
	
	// gpu memcpy
	cudaCheck( cudaMemcpy((void*)dev_llr, p_llr, memsize_llr, cudaMemcpyHostToDevice) );
	cudaCheck( cudaMemcpy((void*)dev_const_llr, p_llr, memsize_llr, cudaMemcpyHostToDevice) );
	//cudaCheck( cudaMemcpy((void*)dev_const_llr, (const void*)p_llr, memsize_llr, cudaMemcpyHostToDevice) );
	
	// gpu argument
	dim3 block(CW, 1, 1);
	dim3 thread(Zc, 1, 1);
	
	// debug info
	int blk, thd;
	fprintf(stderr, "block thread focus: \n");
	scanf("%d%d", &blk, &thd);
	
	ldpc_decoder_gpu<<<block, thread, memsize_llr>>>(BG, dev_llr, dev_const_llr, dev_buf, blk, thd);
	
	int pack = block_length/128;
	pack_decoded_bit<<<pack, 128>>>(dev_llr, dev_tmp);
	
	cudaCheck( cudaMemcpy((void*)decision, (const void*)dev_tmp, block_length*sizeof(unsigned char), cudaMemcpyDeviceToHost) );

	return MAX_ITERATION;
	
}
