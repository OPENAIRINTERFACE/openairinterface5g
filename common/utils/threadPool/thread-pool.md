# Thread pool

The **thread pool** is a working server, made of a set of worker threads that can be mapped on CPU cores.  Each thread pool has an **input queue** ("**FIFO**"), from which its workers pick **jobs** (FIFO element) to execute.  When a job is done, the worker sends a message to an output queue, if it has been defined.

All the thread pool functions are thread safe. The functions executed by worker threads are provided by the thread pool client, so the client has to handle the concurrency/parallel execution of his functions.

## license
    Author:
      Laurent Thomas, Open cells project
      The owner share this piece code to Openairsoftware alliance as per OSA license terms
      With contributions from Robert Schmidt, OSA

# Jobs

A job is a message of type `notifiedFIFO_elt_t`:

* `next`: internal FIFO chain, do not set it
* `key`: a `long int` that the client can use to identify a job or a group of messages
* `ResponseFifo`: if the client defines a response FIFO, the job will be posted back after processing
* `processingFunc`: any function (of type `void processingFunc(void *)`) that a worker will process
* `msgData`: the data passed to `processingFunc`. It can be added automatically, or you can set it to a buffer you are managing
* `malloced`: a boolean that enables internal free in the case of no return FIFO or abort feature

The job messages can be created and destroyed with `newNotifiedFIFO_elt()` and `delNotifiedFIFO_elt()`, or managed by the client:

* `newNotifiedFIFO_elt()`: Creates a job, that will later be used in queues/FIFO
* `delNotifiedFIFO_elt()`: Deletes a job
* `NotifiedFifoData()`: gives a pointer to the beginning of free usage memory in a job (you can put any data there, up to 'size' parameter you passed to `newNotifiedFIFO_elt()`)

These 3 calls are not mandatory, you can also use your own memory to save the `malloc()`/`free()` that are behind these calls.

## API details

### `notifiedFIFO_elt_t *newNotifiedFIFO_elt(int size, uint64_t key, notifiedFIFO_t *reponseFifo, void (*processingFunc)(void *))`

Creates a new job. The data part of the job will have size `size`, the job can be identified via `key`. `reponseFifo` is a FIFO queue to which the job should be pushed after being processed by a thread pool (see below for details) using function `processingFunc()`.

The function returns a pointer to an allocated job.

### `void delNotifiedFIFO_elt(notifiedFIFO_elt_t *elt)`

Free the memory allocated for a job `elt`.

### `void *NotifiedFifoData(notifiedFIFO_elt_t *elt)`

Returns a pointer (`void *`) to the data segment of an allocated job.

# Queues of jobs (`FIFO`)

Queues can be used to enqueue messages/jobs, of type `notifiedFIFO_t`.

* `initNotifiedFIFO()`: Create a queue
* No delete function is required, the creator has only to free the data of type `notifiedFIFO_t`
* `pushNotifiedFIFO()`: Add a job to a queue
* `pullNotifiedFIFO()`: Pull a job from a queue. This call is blocking until a job arrived.
* `pollNotifiedFIFO()`: Pull a job from a queue. This call is not blocking, so it returns always very shortly
* `abortNotifiedFIFO()`: Aborts a FIFO, such that it will always return `NULL`

Note that in 99.9% of cases, `pull()` is better than `poll()`.

The above push/pull/poll functions are built on not-thread-safe versions of these functions that are described below.

## Low level: fast and simple, but not thread-safe

* `initNotifiedFIFO_nothreadSafe()`: Create a queue
* `pushNotifiedFIFO_nothreadSafe()`: Add a element in a queue
* `pullNotifiedFIFO_nothreadSafe()`: Pull a element from a queue

As these queues are not thread safe, there is NO blocking mechanism, neither `pull()` versus `poll()` calls

There is no delete for a message queue: you only have to abandon the memory you allocated to call `initNotifiedFIFO_nothreadSafe(notifiedFIFO_t *nf)`

If you malloced the memory under `nf` parameter, you have to free it.

## FIFO queue abort

It is possible to *abort* the FIFO queue. In this case, any pulling/polling of jobs in the queue will return `NULL`. This can be used when stopping a program, to release multiple queues that might depend on each other (a proper design would try to avoid such dependencies, though).

## API details

### `void initNotifiedFIFO(notifiedFIFO_t *nf)`
### `void initNotifiedFIFO_nothreadSafe(notifiedFIFO_t *nf)`

Initializes a FIFO queue pointed to by `nf` ("notified FIFO").

### `void pushNotifiedFIFO(notifiedFIFO_t *nf, notifiedFIFO_elt_t *msg)`
### `void pushNotifiedFIFO_nothreadSafe(notifiedFIFO_t *nf, notifiedFIFO_elt_t *msg)`

Pushes a job `msg` to the FIFO queue `nf`.

### `notifiedFIFO_elt_t *pullNotifiedFIFO(notifiedFIFO_t *nf)`

Pulls a job from a FIFO queue `nf`. The returned pointer points to the job.  This call is blocking, i.e., a caller will be blocked until there is a message.

If the FIFO has been aborted, the returned pointer will be `NULL`. In other words, if the call to this functions returns `NULL`, the FIFO has been deactivated.

### `notifiedFIFO_elt_t *pollNotifiedFIFO(notifiedFIFO_t *nf)`
### `notifiedFIFO_elt_t *pullNotifiedFIFO_nothreadSafe(notifiedFIFO_t *nf)`

Like `pullNotifiedFIFO()`, but non-blocking: they check if the queue `nf` contains a job, and return it. Otherwise, they return `NULL`.  Note the slight difference in naming: the thread-safe function is called *poll*, not *pull*.

Note that unlike for `pullNotifiedFIFO()`, returning `NULL` does not inform whether the queue has been aborted; the caller should manually check the `abortFIFO` flag of `nf` in this case.

### `void abortNotifiedFIFO(notifiedFIFO_t *nf)`

Aborts the entire FIFO queue `nf`: all jobs will be dropped, and the FIFO is marked as aborted, such that a call to `pullNotifiedFIFO()` returns `NULL`.

Returns the total number of jobs that were waiting or under execution.

# Thread pools

## Initialization

The clients can create one or more thread pools with

* `initTpool(char *params, tpool_t *pool, bool performanceMeas)` or
* `initNamedTpool(char *params,tpool_t *pool, bool performanceMeas , char *name)`

The threads are governed by the Linux real time scheduler, and their name is set automatically to `Tpool<thread index>_<core id>` if `initTpool()` is used or to `<name><thread index>_<core id>` when `initNamedTpool()` is used.

## Adding jobs

The client create their jobs messages as a `notifiedFIFO_elt_t`. They then push it with `pushTpool()` which internally calls `push_notifiedFIFO()`.

If they need a return value (e.g., result of a computation), they have to create response queues with `init_notifiedFIFO()` and set this FIFO pointer in the `notifiedFIFO_elt_t` before pushing the job. This way, multiple result queues can be used in one thread pool.

## Abort

`abortTpool()` kills all jobs in the Tpool, and terminates the pool.

## API details

Each thread pool (there can be several in the same process) should be initialized once using one of the two API's:

### `void initNamedTpool(char *params, tpool_t *pool, bool performanceMeas, char *name)`

### `void initTpool(char *params, tpool_t *pool, bool performanceMeas)`


The `params` string describes a list of cores, separated by "," that run a worker thread.  If the core exists on the CPU, the thread pool initialization sets the affinity between this thread and the related core. The following options are available:

* `N`: if an `N` is in the parameter list, no threads are created, and the threadpool will be disabled (jobs will be treated by each calling thread independently).
* A number that represent a valid CPU core on the target CPU, i.e., if there are `M` cores, a number within `[0,M-1]`: A thread is created and pinned to the core (with set affinity)
* `-1`: The thread will not be mapped onto a specific core (a "floating" thread)

Example: `-1,-1,-1`: the thread pool is made of 3 floating threads.
Be careful with a fixed allocation: it is hard to be more clever than Linux kernel!

`pool` is a pointer to the thread pool object.

`performanceMeas` is a flag to enable measurements (see also below).

`name` is used to build the thread names. 

### `void pushTpool(tpool_t *t, notifiedFIFO_elt_t *msg)`

Adds a job for processing in the thread pool.

The job data you can set are, inside `msg`:

* `key`: an arbitrary key to find a job in a response queue.
* `reponseFifo`: if non-`NULL`, the message will be sent back on this queue when the job is done. If `NULL`, the thread pool automatically frees the job when it is done.
* `processingFunc`: the function to execute for this job.

The `processingFunc` prototype is `void <func>(void *memory)`. The data part of the job (the pointer returned by `NotifiedFifoData(msg)`) is passed to the function. The job data will be updated by `processingFunc`, and the job will be pushed into return queue (the parameter `reponseFifo`).

### `notifiedFIFO_elt_t *pullTpool(notifiedFIFO_t *responseFifo, tpool_t *t)`

Collects a job result in a return queue that has been defined in `msg` when calling `pushTpool()`, and that has been updated by `processingFunc()`. Returns the corresponding job pointer of type `notifiedFIFO_elt_t *`.

Multiple return queues might be useful. Consider the following example in the eNB: I created one single thread pool (because it depends mainly on CPU hardware), but i use two return queues: one for turbo encode, one for turbo decode. Thus, when performing Turbo encoding/decoding, jobs are pushed into a single thread pool, but will end up in different result queues, such that DL/UL processing can be separated more easily.

### `notifiedFIFO_elt_t *tryPullTpool(notifiedFIFO_t *responseFifo, tpool_t *t)`

The same as `pullTpool()` in a non-blocking fashion (an alternative name would have been `pollTpool()`).

### `int abortTpool(tpool_t *t)`

Aborts the complete Tpool: cancel every work in the input queue, marks to drop existing jobs in processing, and terminates all worker threads. It is afterwards still possible to call functions such as `pushTpool()`, but each calling thread will execute the job itself.

Returns the total number of jobs that were aborted, i.e., waiting for execution or being executed.

## Performance measurements

A performance measurement is integrated: the pool will automacillay fill timestamps if you set the environement variable `OAI_THREADPOOLMEASUREMENTS` to a valid file name.  The following measurements will be written to Linux pipe on a per-job basis:

* `creationTime`: time the request is push to the pool;
* `startProcessingTime`: time a worker start to run on the job
* `endProcessingTime`: time the worker finished the job
* `returnTime`: time the client reads the result

The `measurement_display` tool to read the Linux pipe and display it in ASCII is provided.
In the cmake build directory, type `make/ninja measurement_display`. Use as
follows:
```
sudo OAI_THREADPOOLMEASUREMENTS=tpool.meas ./nr-softmodem ...
./measurement_display tpool.meas
```
