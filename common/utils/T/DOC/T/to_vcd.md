# to_vcd

This tracer is used to dump a VCD trace of the softmodem.

The basic usage is:

```shell
cd common/utils/T/tracer
./to_vcd -d ../T_messages.txt -vcd -o /tmp/openair_dump_eNB.vcd
```

Apart from VCD specific traces, you can dump to VCD other T traces.
Use `-b` and `-l` options. For example, to only log DLSCH scheduler VCD traces, use:
```shell
./to_vcd -d ../T_messages.txt -o /tmp/openair_dump_eNB.vcd -b VCD_FUNCTION_SCHEDULE_DLSCH value mac_schedule_dlsch
```

For more help, run:
```shell
./to_vcd -h
```
