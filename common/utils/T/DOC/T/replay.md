# Replay

First, read the [basic usage](./basic.md) to compile things.

Then, read [record](./record.md) to know how to record a trace.

To replay, you use the `replay` program. It will act
as a *tracee* (`lte-softmodem`, `oaisim`, etc.).

Then you use your usual tracer, eg. `enb` or `textlog`.

Since the T is constantly evolving you may need to extract
`T_messages.txt` contained in a trace to be able to process
the trace.

## Example

Download [example.raw](./example.raw). This example contains:
* MAC PDUs as sent and received by the eNodeB
* RRC info messages the eNodeB produced.

First step is to extract `T_messages.txt` from this trace.

```shell
./extract_config -i example.raw > extracted_T_messages.txt
```

Then you use `replay` to act as a regular *tracee*:

```shell
./replay -i example.raw
```

And then, while `replay` is still running, you use the tracer
you want, for example the `textlog` tracer:

```shell
./textlog -d extracted_T_messages.txt -no-gui -ON -full
```
