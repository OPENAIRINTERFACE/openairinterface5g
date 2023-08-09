# T tracer - a simple tutorial

There is a diff file with all modifications:
[howto_new_trace.patch](./howto_new_trace.patch).
We use tag 2023.w28
To use the patch, clone the repository,
then in the top directory of openairinterface:
```
git checkout 2023.w28
patch -p1 < common/utils/T/DOC/T/howto_new_trace.patch
```

Then compile nr-softmodem as usual.

## 1 - create a new trace

As an example, let's add a new trace to dump LDPC decoding success.

In the file `common/utils/T/T_messages.txt`
we add those two lines:

```
ID = LDPC_OK
    FORMAT = int,segment : int,nb_segments : int,offset : buffer,data
```

- You create an ID for the new trace.
- You give the format as a list of variables with there types.

Then, in the file `openair1/SCHED_NR/phy_procedures_nr_gNB.c`,
in function `nr_postDecode()`, we add, at the place where we want to trace:

```
    T(T_LDPC_OK,
      T_INT(rdata->segment_r),
      T_INT(rdata->nbSegments),
      T_INT(rdata->offset),
      T_BUFFER(ulsch_harq->c[r], rdata->Kr_bytes - (ulsch_harq->F>>3) -((ulsch_harq->C>1)?3:0)));
```

And that's all, the code is instrumented, a trace is created,
we now need a tracer to process the trace.

## 2. create a tracer

See `trace_ldpc.c` in the attached diff. Read it, there is documentation.
To compile, just do `make trace_ldpc` in `common/utils/T/tracer`
To use: `cd common/utils/T/tracer; ./trace_ldpc -d ../T_messages.txt`

Your tracer can do whatever it wants with the data.

The Makefile has been modified to compile it. See the diff.

Use it as a basis for your own tracer.
You can also look at the other tracers, especially `textlog.c`
and `macpdu2wireshark.c` which are the most useful for me to
trace/debug the nr-softmodem.

## 3. available types in a trace

You trace the code with:

```
    T(T_trace_name, T_xxx(variable1), T_xxx(variables2), ...);
```

`T_xxx` can be:

- `T_INT(variable)`
- `T_FLOAT(variable)`                  (not tested much, may fail)
- `T_BUFFER(variable, length)`
- `T_STRING(variable)`
- `T_PRINTF(format, arg1, ... argn)`

What is traced is obvious from the name. `T_INT` traces an int for example.

corresponding type to put in `T_messages.txt`

- `T_INT`: int
- `T_FLOAT`: float
- `T_BUFFER`: buffer
- `T_STRING`: string
- `T_PRINTF`: string

And that's all.
You can add as many traces as you want, just add them in `T_messages.txt`,
put the `T()` macro in the code where you want to trace, and modify your
tracer to deal with it.

An alternative to writing your own tracer could be to use `textlog`
with `-full` command line and process the text with some external
tools.

It is also possible to record the events in a file using the tracer
`record` and then use `replay` to process the recorded file later.
