/*
 * Tsync.h
 *
 */
#include <pthread.h>
#ifndef TSYNC_H_
#define TSYNC_H_

#define COMPILER_BARRIER() __asm__ __volatile__ ("" ::: "memory")
int mycount;
pthread_mutex_t exclusive;
pthread_mutex_t downlink_mutex[MAX_eNB][MAX_UE];
pthread_mutex_t downlink_mutex_channel;
pthread_cond_t downlink_cond[MAX_eNB][MAX_UE];
pthread_cond_t downlink_cond_channel;

pthread_mutex_t uplink_mutex[MAX_UE][MAX_eNB];
pthread_mutex_t uplink_mutex_channel;
pthread_cond_t uplink_cond[MAX_UE][MAX_eNB];
pthread_cond_t uplink_cond_channel;

int COT;
int COT_U;

volatile int _COT;
volatile int _COT_U;


int NUM_THREAD_DOWNLINK;
int NUM_THREAD_UPLINK;

ch_thread *e2u_t[MAX_UE][MAX_eNB];
ch_thread *u2e_t[MAX_UE][MAX_eNB];

pthread_t cthr_u[MAX_eNB][MAX_UE];
pthread_t cthr_d[MAX_eNB][MAX_UE];

int fd_NB[MAX_eNB+MAX_UE];
int fd_channel;


#endif /* TSYNC_H_ */
