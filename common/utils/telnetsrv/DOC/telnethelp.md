# oai telnet server global help

``` bash
alblf@at8020-a:~$ telnet 10.133.10.77 9090
Trying 10.133.10.77...
Connected to 10.133.10.77.
Escape character is '^]'.

softmodem> help
   module 0 = telnet:
      telnet [get set] debug <value>
      telnet [get set] prio <value>
      telnet [get set] loopc <value>
      telnet [get set] loopd <value>
      telnet [get set] phypb <value>
      telnet [get set] hsize <value>
      telnet [get set] hfile <value>
      telnet redirlog [here,file,off]
      telnet param [prio]
      telnet history [list,reset]
   module 1 = softmodem:
      softmodem show loglvl|thread|config
      softmodem log (enter help for details)
      softmodem thread (enter help for details)
      softmodem exit
   module 2 = phy:
      phy disp [phycnt,uedump,uestat UE<x>]
   module 3 = loader:
      loader [get set] mainversion <value>
      loader [get set] defpath <value>
      loader [get set] maxshlibs <value>
      loader [get set] numshlibs <value>
      loader show [params,modules]
   module 4 = coding:
      coding [get set] maxiter <value>
      coding mode [sse,avx2,stdc,none]
softmodem>
```

# oai telnet server, specific commands help

``` bash
softmodem> softmodem log help
 log sub commands:
 show:  		     display current log configuration
 online, noonline:	     enable or disable console logs
 enable, disable id1-id2:    enable or disable logs for components index id1 to id2
 level_<level> id1-id2:      set log level to <level> for components index id1 to id2
 level_<verbosity> id1-id2:  set log verbosity to <verbosity> for components index id1 to id2
use the show command to get the values for <level>, <verbosity> and the list of component indexes that can be used for id1 and id2
softmodem>


```

[oai telnetserver home](telnetsrv.md)
[oai telnetserver usage home](telnetusage.md)
