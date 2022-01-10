/*
  queue_t is a basic thread-safe non-blocking fixed-size queue.

  put_queue returns false if the queue is full.
  get_queue returns NULL if the queue is empty.

  Example usage:

    // Initialization:
    queue_t fooq;
    init_queue(&fooq);

    // Producer:
    foo_t *item = new_foo();
    if (!put_queue(&fooq, item))
      delete_foo(item);

    // Consumer:
    foo_t *item = get_queue(&fooq);
    if (item)
    {
      do_something_with_foo(item);
      delete_foo(item);
    }
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define MAX_QUEUE_SIZE 512

typedef struct queue_t
{
    void *items[MAX_QUEUE_SIZE];
    size_t read_index, write_index;
    size_t num_items;
    pthread_mutex_t mutex;
} queue_t;

void init_queue(queue_t *q);
void reset_queue(queue_t *q);
void *put_queue_replace(queue_t *q, void *item);
bool put_queue(queue_t *q, void *item);
void *get_queue(queue_t *q);

/* Put the given item back onto this queue at the head.
   (The next call to put_queue would return this item.)
   Return true if successful, false if the queue was full */
bool requeue(queue_t *q, void *item);

/* Remove the last item queued.
   Return the item or NULL if the queue was empty */
void *unqueue(queue_t *q);

typedef bool queue_matcher_t(void *wanted, void *candidate);

/* Unqueue the most recently queued item for which `matcher(wanted, candidate)`
   returns true where `candidate` is an item currently on the queue.
   Look only at the last `max_depth` items on the queue, at most.
   Returns the candidate item, or NULL if none matches */
void *unqueue_matching(queue_t *q, size_t max_depth, queue_matcher_t *matcher, void *wanted);
