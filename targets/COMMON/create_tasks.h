#ifndef CREATE_TASKS_H_
#define CREATE_TASKS_H_

#if defined(ENABLE_ITTI)
/* External declaration of L2L1 task that depend on the target */
extern void *l2l1_task(void *arg);

int create_tasks(uint32_t enb_nb, uint32_t ue_nb);
#endif

#endif /* CREATE_TASKS_H_ */
