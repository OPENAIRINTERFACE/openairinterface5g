# logging facility source files

The oai logging facility is implemented in two source files, located in [common/utils/LOG](LOG)
1. [log.c](../log.c) contains logging implementation
1.  [log.h](../log.h) is the logging facility include file containing both private and public data type definitions. It also contain API prototypes.

The logging facility doesn't create any thread, all api's are executed in the context of the caller. The tracing macro's `LOG_<X>` are all using the logRecord_mt function to output the messages. To keep this function thread safe it must perform a single system call to the output stream. The buffer used to build the message must be specific to the calling thread, which is today enforced by using a variable in the logRecord_mt stack.

Data used by the logging utility are defined by the `log_t` structure which is allocated at init time, when calling the `logInit` function.

[logging facility  main page](log.md)
[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
