The telnet server includes a **_loop_** command that can be used to iterate a given command. The number of iterations and the delay, in ms between two iterations can be modified, as shown in the following example:

```bash
softmodem> telnet get loopc
telnet, loopc = 10
softmodem> telnet get loopd
telnet, loopd = 2000
softmodem> telnet set loopd 1000
telnet, loopd set to
1000
softmodem> loop softmodem show thread
                  2018-03-27 17:58:49.000 2/10
  id          name            state   USRmod    KRNmod  prio nice   vsize   proc pol

     3946        lte-softmodem   S       20005      9440  20   0 236560384   2    0 other

     3946        lte-softmodem   S           7        95  20   0 236560384   2    0 other
     3948               telnet   R           0         0  20   0 236560384   2    0 other
     3949        ITTI acceptor   S           2         9  20   0 236560384   2    0 other
     3951              ITTI 12   S           2         2  20   0 236560384   7    0 other
     3952              ITTI 11   S           0         0  20   0 236560384   0    0 other
     3953               ITTI 9   S           0         0  20   0 236560384   1    0 other
     3954               ITTI 7   S           0         0  20   0 236560384   7    0 other
     3955               ITTI 8   S           0         0  20   0 236560384   7    0 other
     3956               ITTI 4   S          35         0  20   0 236560384   2    0 other
     3957            ru_thread   S       15366      3072 -10   0 236560384   0    2 rt: rr
     3958      ru_thread_prach   S           0         0 -10   0 236560384   7    1 rt: fifo
     3959           fep_thread   S        1874       123 -10   0 236560384   5    1 rt: fifo
     3960         feptx_thread   S        1554       101 -10   0 236560384   7    1 rt: fifo
     3969            ru_thread   S           0         0 -10   0 236560384   0    2 rt: rr
     3970            ru_thread   S        1313      5522 -10   0 236560384   5    2 rt: rr
     3971            ru_thread   S           4         6 -10   0 236560384   1    2 rt: rr
     3972        lte-softmodem   S         318         9 -10   0 236560384   7    1 rt: fifo
     3973        lte-softmodem   S           6        13 -10   0 236560384   4    1 rt: fifo

```
A **_loop_** command can be interrupted by pressing the  **_enter_** key till getting the  prompt.

[oai telnetserver home](telnetsrv.md)
[oai telnetserver usage home](telnetusage.md)
