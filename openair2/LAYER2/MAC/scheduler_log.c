#include "scheduler_log.h"
#include "common/utils/LOG/log.h"
#include <stdio.h>
#include <time.h>

FILE *scheduler_csv = NULL;

void scheduler_log_init(void)
{
  scheduler_csv = fopen("/openairinterface5g/logs/scheduler_log.csv", "w");

  if (scheduler_csv == NULL) {
    LOG_E(MAC, "[SCHED_LOG] ERROR: Cannot open CSV log file.\n");
    return;
  }

  // Write the CSV header
  fprintf(scheduler_csv,
          "timestamp_ms,frame,subframe,rnti,direction,"
          "nb_rb,mcs,tbs_bytes,cqi,retx\n");

  fflush(scheduler_csv);
  LOG_I(MAC, "[SCHED_LOG] CSV loggin started, writing to /openairinterface5g/logs/scheduler_log.csv\n");
}

void scheduler_log_close(void) {
  if (scheduler_csv) {
    fclose(scheduler_csv);
    scheduler_csv = NULL;
    LOG_I(MAC, "[SCHED_LOG] CSV logging stopped.\n");
  }
}
