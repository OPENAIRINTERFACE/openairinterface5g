# Record

First, read the [basic usage](./basic.md) to compile things.

To record, you use the `record` tracer.

To send a trace to Eurecom, you run (unless we specifically ask you
to activate/deactivate specific traces):

```shell
./record -d ../T_messages.txt -o record.raw -ON -off VCD -off HEAVY -off LEGACY_GROUP_TRACE -off LEGACY_GROUP_DEBUG
```

And then you run the program to trace (`lte-softmodem`, `oaisim`, whatever)
as explained in [basic usage](./basic.md).

To stop the recording, you simply press `control+c` to end `record`.

You send to Eurecom the file `record.raw`. The file `T_messages.txt` is not needed.

To get a list of options, run:

```shell
./record -h
```

The options `-ON`, `-OFF`, `-on` and `-off` are processed in order.
In the example above we first activate all the traces (`-ON`) and then
deactivate specific ones (more precisely, in this particular case:
the *groups* of traces `VCD`, `HEAVY`, `LEGACY_GROUP_TRACE` and
`LEGACY_GROUP_DEBUG`).

You can then [replay](./replay.md) the recorded file or send it to others so they
can analyse things.
