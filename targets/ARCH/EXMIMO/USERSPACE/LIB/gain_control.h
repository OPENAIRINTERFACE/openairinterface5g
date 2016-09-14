#define TARGET_RX_POWER 50    // Target digital power for the AGC
#define TARGET_RX_POWER_MAX 53    // Maximum digital power for AGC
#define TARGET_RX_POWER_MIN 48    // Minimum digital power for AGC
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#endif

void gain_control_all (unsigned int rx_power_fil_dB, unsigned int card);
void gain_control (unsigned int rx_power_fil_dB, unsigned int ant, unsigned int card);
