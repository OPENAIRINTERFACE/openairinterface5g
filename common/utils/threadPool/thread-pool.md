# Thread pool

The thread pool is a working server, made of a set of worker threads that can be mapped on CPU cores.

Each worker loop on pick from the same input queue jobs to do.
When a job is done, the worker sends a return if a return is defined.

A selective abort allows to cancel parallel jobs (usage: a client pushed jobs, but from a response of one job, the other linked jobs becomes useless).

All the thread pool functions are thread safe, nevertheless the working functions are implemented by the thread pool client, so the client has to tackle the parallel execution of his functions called "processingFunc" hereafter.

## license
Author: Laurent Thomas, Open cells project 
The owner share this piece code to Openairsoftware alliance as per OSA license terms

# jobs

A job is a message (notifiedFIFO_elt_t):
next: internal FIFO chain, do not set it
key:  a long int that the client can use to identify a message or a group of messages
responseFifo: if the client defines a response FIFO, the message will be posted back after processing
processingFunc: any funtion (type void processingFunc(void *)) that the worker will launch
msgData: the data passed to processingFunc. It can be added automatically, or you can set it to a buffer you are managing
malloced: a boolean that enable internal free in these cases: no return Fifo or Abort feature

The job messages can be created with newNotifiedFIFO_elt() and delNotifiedFIFO_elt() or managed by the client.

# Queues of jobs

Queues are type of: notifiedFIFO_t that must be initialized by init_notifiedFIFO()
No delete function is required, the creator has only to free the data of type notifiedFIFO_t

push_notifiedFIFO() add a job in the queue
pull_notifiedFIFO() is blocking, poll_notifiedFIFO() is non blocking

abort_notifiedFIFO() allows the customer to delete all waiting jobs that match with the key (see key in jobs definition)

# Thread pools

## initialization
The clients can create one or more thread pools with init_tpool()
the params string structure: describes a list of cores, separated by "," that run a worker thread

If the core exists on the CPU, the thread pool initialization sets the affinity between this thread and the related code (use negative values is allowed, so the thread will never be mapped on a specific core).

The threads are all Linux real time scheduler, their name is set automatically is "Tpool_<core id>"

## adding jobs
The client create their jobs messages as a notifiedFIFO_elt_t, then they push it with pushTpool() (that internally calls push_notifiedFIFO())

If they need a return, they have to create response queues with init_notifiedFIFO() and set this FIFO pointer in the notifiedFIFO_elt_t before pushing the job.

## abort

A abort service abortTpool() allows to abort all jobs that match a key (see jobs "key"). When the abort returns, it garanties no job (matching the key) response will be posted on response queues.

Nevertheless, jobs already performed before the return of abortTpool() are pushed in the response Fifo queue.

## Performance measurements

A performance measurement is integrated: the pool will automacillay fill timestamps:

* creationTime: time the request is push to the pool;
* startProcessingTime: time a worker start to run on the job
* endProcessingTime: time the worker finished the job
* returnTime: time the client reads the result

if you set the environement variable: thread-pool-measurements to a valid file name
These measurements will be wrote to this Linux pipe.

A tool to read the linux fifo and display it in ascii is provided: see the local directory Makefile for this tool and to compile the thread pool unitary tests.
