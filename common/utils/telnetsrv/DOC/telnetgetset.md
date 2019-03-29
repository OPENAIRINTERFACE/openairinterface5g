getall command can be used to get the list of variables that can bet set or get from the telnet shell. Knowing the names of the variables they can then be set or read. Setting a variable is not always relevant, the telnet server doesn't provide a mechanism to restrict the set command.

```bash
softmodem> telnet getall
telnet, debug = 0
telnet, prio = 0
telnet, loopc = 10
telnet, loopd = 5000
telnet, phypb = 65000
telnet, hsize = 50
telnet, hfile = "oaitelnet.history"
softmodem> telnet set loopc 100
telnet, loopc set to
100
softmodem> telnet get loopc
telnet, loopc = 100
softmodem>

```
[oai telnetserver home](telnetsrv.md)
[oai telnetserver usage home](telnetusage.md)
