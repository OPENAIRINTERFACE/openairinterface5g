# Thread pool

The thread pool is a working server, made of a set of worker threads that can be mapped on CPU cores.

Each worker loop on pick from the same input queue jobs to do.

When a job is done, the worker sends a return if a return is defined.

A selective abort allows to cancel parallel jobs (usage: a client pushed jobs, but from a response of one job, the other linked jobs becomes useless).

All the thread pool functions are thread safe, nevertheless the working functions are implemented by the thread pool client, so the client has to tackle the parallel execution of his functions called "processingFunc" hereafter.

## license
    Author:
      Laurent Thomas, Open cells project
      The owner share this piece code to Openairsoftware alliance as per OSA license terms

# jobs

A job is a message (`notifiedFIFO_elt_t`):  

* `next`:
  internal FIFO chain, do not set it
* `key`:
  a long int that the client can use to identify a message or a group of messages
* `ResponseFifo`:
  if the client defines a response FIFO, the message will be posted back after processing
* `processingFunc`:
  any funtion (type `void processingFunc(void *)`) that the worker will launch
* `msgData`:
  the data passed to processingFunc. It can be added automatically, or you can set it to a buffer you are managing
* `malloced`:
  a boolean that enable internal free in these cases:
  no return Fifo or Abort feature

The job messages can be created with `newNotifiedFIFO_elt()` and `delNotifiedFIFO_elt()` or managed by the client.

# Queues of jobs

Queues are type of:

* `notifiedFIFO_t` that must be initialized by `init_notifiedFIFO()`
* No delete function is required, the creator has only to free the data of type `notifiedFIFO_t`
* `push_notifiedFIFO()` add a job in the queue
* `pull_notifiedFIFO()` is blocking, `poll_notifiedFIFO()` is non blocking
* `abort_notifiedFIFO()` allows the customer to delete all waiting jobs that match with the key (see key in jobs definition)

## Thread-safe functions

* `newNotifiedFIFO_elt()`: creates a message, that will later be used in queues/FIFO
* `delNotifiedFIFO_elt()`: deletes it
* `NotifiedFifoData()`: gives a pointer to the beginning of free usage memory in a message (you can put any data there, up to 'size' parameter you passed to `newNotifiedFIFO_elt()`)

These 3 calls are not mandatory, you can also use your own memory to save the `malloc()`/`free()` that are behind these calls

# Low level: fast and simple, but not thread-safe

* `initNotifiedFIFO_nothreadSafe()`: Create a queue
* `pushNotifiedFIFO_nothreadSafe()`: Add a element in a queue
* `pullNotifiedFIFO_nothreadSafe()`: Pull a element from a queue

As these queues are not thread safe, there is NO blocking mechanism, neither `pull()` versus `poll()` calls

There is no delete for a message queue: you only have to abandon the memory you allocated to call `initNotifiedFIFO_nothreadSafe(notifiedFIFO_t *nf)`

So if you malloced the memory under 'nf' parameter you have to free it, if it is automatic variable (local variable) or global variable, nothing is to be done.


## thread safe queues

These queues are built on not thread safe queues when we need thread to thread protection

* `initNotifiedFIFO()`: Create a queue
* `pushNotifiedFIFO()`: Add a element in a queue
* `pullNotifiedFIFO()`: Pull a element from a queue, this call is blocking until a message arrived
* `pollNotifiedFIFO()`: Pull a element from a queue, this call is not blocking, so it returns always very shortly

Note that in 99.9% cases, `pull()` is better than `poll()`

No `delete()` call, same principle as not thread safe queues

# Thread pools

## initialization

The clients can create one or more thread pools with `initTpool(char *params,tpool_t *pool, bool performanceMeas  )` or `initNamedTpool(char *params,tpool_t *pool, bool performanceMeas , char *name)`

the `params` string structure: describes a list of cores, separated by "," that run a worker thread

If the core exists on the CPU, the thread pool initialization sets the affinity between this thread and the related code (use negative values is allowed, so the thread will never be mapped on a specific core).

The threads are all Linux real time scheduler, their name is set automatically to `Tpool<thread index>_<core id>` if initTpool is used or to `<name><thread index>_<core id>` when initNamedTpool is used.

## adding jobs
The client create their jobs messages as a `notifiedFIFO_elt_t`, then they push it with `pushTpool()` (that internally calls `push_notifiedFIFO()`)

If they need a return, they have to create response queues with `init_notifiedFIFO()` and set this FIFO pointer in the `notifiedFIFO_elt_t` before pushing the job.

## abort

A abort service `abortTpoolJob()` allows to abort all jobs that match a key (see jobs "key"). When the abort returns, it garanties no job (matching the key) response will be posted on response queues.

Nevertheless, jobs already performed before the return of `abortTpoolJob()` are pushed in the response Fifo queue.

`abortTpool()` kills all jobs in the Tpool.

## API details
Each thread pool (there can be several in the same process) should be initialized once using one of the two API's:

### `initNamedTpool(char *params,tpool_t *pool, bool performanceMeas,char *name)`

### `initTpool(char *params,tpool_t *pool, bool performanceMeas)`

`params`: the configuration parameter is a string, elements separator is a comma ",". An element can be:

* `N`: if a N is in the parameter list, no threads are created
    The purpose is to keep the same API in any case
* a CPU with a little number of cores,
    or in debugging sessions to simplify the human work
* a number that represent a valid CPU core on the target CPU
    A thread is created and stick on the core (with set affinity)
* a number that is not a valid CPU core
    a floating thread is created (Linux is responsible of the real time core allocation)

example: `"-1,-1,-1"`
as there is no core number -1, the thread pool is made of 3 floating threads
be careful with fix allocation: it is hard to be more clever than Linux kernel

`pool` is the memory you allocated before to store the thread pool internal state (same concept as above queues)

`performanceMeas` is a flag to enable measurements (well described in documentation)

`name` is used to build the thread names. 

### `pushTpool(tpool_t *t, notifiedFIFO_elt_t *msg)`

adds a job to do in the thread pool

The msg data you can set are:

* `key`: a value for you that you will find back in the response it is also the key for `abortTpoolJob()`
* `reponseFifo`: if you set it, the message will be sent back on this queue when the job is done if you don't set it, no return is performed, the thread pool frees the message 'msg' when the job is done
* `processingFunc`: the function the job will run. the function prototype is `void <func>(void *memory)` the data part (the pointer returned by `NotifiedFifoData(msg)`) is passed to the function it is used to send data to the processing function and also to write back results of course, writing back results will lead you to use also a return queue (the parameter `reponseFifo`)

### `pullTpool()` collects job result in a return queue

you collect results in one result queue: the message you gave to `pushTpool()`, nevertheless it has been updated by `processingFunc()`

An example of multiple return queues, in eNB: I created one single thread pool (because it depends mainly on CPU hardware), but i use two return queues: one for turbo encode, one for turbo decode.

### `tryPullTpool()`

is the same, but not blocking (`pollTpool()` would have been a better name)

### `abortTpoolJob()`

Is a facility to cancel work you pushed to a thread pool

I used it once: when eNB performs turbo decode, I push all segments in the thread pool.

But when I get back the decoding results, if one segment can't be decoded, I don't need the results of the other segments of the same UE.

## Performance measurements

A performance measurement is integrated: the pool will automacillay fill timestamps if you set the environement variable `threadPoolMeasurements` to a valid file name.  The following measurements will be written to Linux pipe.

* `creationTime`: time the request is push to the pool;
* `startProcessingTime`: time a worker start to run on the job
* `endProcessingTime`: time the worker finished the job
* `returnTime`: time the client reads the result

The `measurement_display` tool to read the Linux pipe and display it in ascii is provided.
In the cmake build directory, type `make/ninja measurement_display`. Use as
follows:
```
sudo threadPoolMeasurements=tpool.meas ./nr-softmodem ...
./measurement_display tpool.meas
```
