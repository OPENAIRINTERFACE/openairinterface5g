#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <complex>
#include <cmath>
#include <map>
#include <PHY/MODULATION/modulation_extern.h>
using namespace std;
extern "C" {
  void gen_sig(int RB, int len, double ratio, int16_t *table_n, int16_t *table_e ) {
    double samplerate = 30.72e6*ratio;
    double ofdm_size = 2048*ratio;
    double PI = std::acos(-1);
    complex<double> t[len];
    int index=0;
    double cp0 = 160*ratio;
    double cp = 144*ratio;

    for (int i=-cp0; i<ofdm_size; i++)
      t[index++] = polar( 32767.0, -2*PI*i*7.5e3/samplerate);

    for(int x=0 ; x <6 ; x++)
      for (int i=-cp; i<ofdm_size; i++)
        t[index++] = polar( 32767.0, -2*PI*i*7.5e3/samplerate);

    for (int i=0; i < len ; i++) {
      table_n[i*2] = floor(real(t[i]));
      table_n[i*2+1] = floor(imag(t[i]));
    }

    index=0;
    double cpe = 512*ratio;

    for(int x=0 ; x <6 ; x++)
      for (int i=-cpe; i<ofdm_size; i++)
        t[index++] = polar( 32767.0, -2*PI*i*7.5e3/samplerate);

    for (int i=0; i < len ; i++) {
      table_e[i*2] = floor(real(t[i]));
      table_e[i*2+1] = floor(imag(t[i]));
    }
  }

  int16_t *s6n_kHz_7_5;
  int16_t *s6e_kHz_7_5;
  int16_t *s15n_kHz_7_5;
  int16_t *s15e_kHz_7_5;
  int16_t *s25n_kHz_7_5;
  int16_t *s25e_kHz_7_5;
  int16_t *s50n_kHz_7_5;
  int16_t *s50e_kHz_7_5;
  int16_t *s75n_kHz_7_5;
  int16_t *s75e_kHz_7_5;
  int16_t *s100n_kHz_7_5;
  int16_t *s100e_kHz_7_5;
  int16_t **tables[12]= {&s6n_kHz_7_5,&s6e_kHz_7_5,
                         &s15n_kHz_7_5,&s15e_kHz_7_5,
                         &s25n_kHz_7_5,&s25e_kHz_7_5,
                         &s50n_kHz_7_5,&s50e_kHz_7_5,
                         &s75n_kHz_7_5,&s75e_kHz_7_5,
                         &s100n_kHz_7_5,&s100e_kHz_7_5,
                        };
  int tables_size_bytes[12];
#define MyAssert(x) { if(!(x)) { printf("Error in table intialization: %s:%d\n",__FILE__,__LINE__); exit(1);}}
  void init_7_5KHz(void) {
    const map<int,double> tables_7_5KHz= {{6,1.0/16},{15,1.0/8},{25,1.0/4},{50,1.0/2},{75,3.0/4},{100,1.0}};
    int tables_idx=0;

    for (auto it=tables_7_5KHz.begin(); it!=tables_7_5KHz.end(); ++it) {
      int len=15360*it->second;
      tables_size_bytes[tables_idx]=sizeof(int16_t)*2*len;
      tables_size_bytes[tables_idx+1]=sizeof(int16_t)*2*len;
      MyAssert(0==posix_memalign((void **)tables[tables_idx],
                                 16,tables_size_bytes[tables_idx]));
      MyAssert(0==posix_memalign((void **)tables[tables_idx+1],
                                 16,tables_size_bytes[tables_idx+1]));
      gen_sig(it->first, len, it->second, *tables[tables_idx], *tables[tables_idx+1]);
      tables_idx+=2;
    }
  }
}
