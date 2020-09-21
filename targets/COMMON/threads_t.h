#ifndef _TARGETS_COMMON_THREADS_T_H_
#define _TARGETS_COMMON_THREADS_T_H_

typedef struct threads_s {
  int main;
  int sync;
  int one;
  int two;
  int three;
  int slot1_proc_one;
  int slot1_proc_two;
  int slot1_proc_three;
  //int dlsch_td_one;
  //int dlsch_td_two;
  //int dlsch_td_three;
  //int dlsch_td1_one;
  //int dlsch_td1_two;
  //int dlsch_td1_three;
} threads_t;

#endif /* _TARGETS_COMMON_THREADS_T_H_ */
