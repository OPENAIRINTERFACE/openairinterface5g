# OAI console logging facility

oai includes a console logging facility that any component should use when writting informational or debugging messages to the softmodem or uesoftmodem stdout stream.
By default, this facility is included at build-time and activated at run-time. The T Tracer and the Logging facility share common options for activation:

-   To disable the logging facility (and T Tracer) at build-time use the *--disable-T-Tracer* switch:

```bash
/build_oai --disable-T-Tracer
```
-  To use the the T-Tracer instead of the console logging facility, use the command line option *T_stdout*.  *T_stdout* is a boolean option, which, when set to 0 (false) disable the console logging facility. All stdout messages are then sent to the T-Tracer.

## Documentation

* [runtime usage](rtusage.md)
* [developer usage](devusage.md)
* [module architecture](arch.md)

[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
