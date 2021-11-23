#include "queue_t.h"
#include <stdio.h>
#include <stdlib.h>

#define FAIL do {                                                       \
    printf("\n*** FAILED at %s line %d\n", __FILE__, __LINE__);         \
    pass = false;                                                       \
} while (0)

#define EQUAL(A, B) do {                                                \
    if ((A) != (B))                                                     \
        FAIL;                                                           \
} while (0)

typedef uint32_t Thing_t;       /* actual type doesn't matter */

static Thing_t things[MAX_QUEUE_SIZE];
static Thing_t thing1, thing2;

static bool matcher(void *wanted, void *candidate)
{
    return wanted == candidate;
}

int main(void)
{
    bool pass = true;
    queue_t queue;
    init_queue(&queue);

    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        if (!put_queue(&queue, &things[i]))
        {
            FAIL;
        }
    }

    /* queue is full */
    if (put_queue(&queue, &thing1))
        FAIL;

    Thing_t *p;
    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        p = get_queue(&queue);
        EQUAL(p, &things[i]);
    }

    /* queue is empty */
    p = get_queue(&queue);
    EQUAL(p, NULL);

    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        if (!put_queue(&queue, &things[i]))
        {
            FAIL;
        }
    }

    p = get_queue(&queue);
    EQUAL(p, &things[0]);

    p = get_queue(&queue);
    EQUAL(p, &things[1]);

    if (!requeue(&queue, &thing1))
        FAIL;
    if (!requeue(&queue, &thing2))
        FAIL;
    p = get_queue(&queue);
    EQUAL(p, &thing2);
    p = get_queue(&queue);
    EQUAL(p, &thing1);

    if (!requeue(&queue, &things[1]))
        FAIL;
    if (!requeue(&queue, &things[0]))
        FAIL;

    for (int i = 0; i < MAX_QUEUE_SIZE / 2; ++i)
    {
        p = get_queue(&queue);
        EQUAL(p, &things[i]);
    }

    for (int i = MAX_QUEUE_SIZE / 2; i < MAX_QUEUE_SIZE; ++i)
    {
        if (!put_queue(&queue, &things[i]))
            FAIL;
    }

    p = get_queue(&queue);
    EQUAL(p, &things[MAX_QUEUE_SIZE / 2]);
    p = get_queue(&queue);
    EQUAL(p, &things[MAX_QUEUE_SIZE / 2 + 1]);

    // ---- unqueue ----

    init_queue(&queue);
    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        if (!put_queue(&queue, &things[i]))
        {
            FAIL;
        }
    }
    for (int i = MAX_QUEUE_SIZE; --i >= 0;)
    {
        p = unqueue(&queue);
        EQUAL(p, &things[i]);
        EQUAL(queue.num_items, i);
    }
    EQUAL(queue.num_items, 0);
    if (!put_queue(&queue, &thing1))
        FAIL;
    if (!put_queue(&queue, &thing2))
        FAIL;
    EQUAL(queue.num_items, 2);
    p = get_queue(&queue);
    EQUAL(p, &thing1);
    p = get_queue(&queue);
    EQUAL(p, &thing2);

    // ---- unqueue_matching ----

    init_queue(&queue);

    // empty queue
    p = unqueue_matching(&queue, MAX_QUEUE_SIZE, matcher, &thing1);
    EQUAL(p, NULL);
    EQUAL(queue.num_items, 0);

    // one item in queue
    if (!put_queue(&queue, &thing1))
        FAIL;
    EQUAL(queue.num_items, 1);
    p = unqueue_matching(&queue, MAX_QUEUE_SIZE, matcher, &thing2);
    EQUAL(p, NULL);
    EQUAL(queue.num_items, 1);

    p = unqueue_matching(&queue, /*max_queue=*/ 0, matcher, &thing1);
    EQUAL(p, NULL);
    EQUAL(queue.num_items, 1);

    p = unqueue_matching(&queue, /*max_queue=*/ 1, matcher, &thing1);
    EQUAL(p, &thing1);
    EQUAL(queue.num_items, 0);

    // more max_queue values
    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        if (!put_queue(&queue, &things[i]))
        {
            FAIL;
        }
    }
    p = unqueue_matching(&queue, /*max_queue=*/ 0, matcher, &things[MAX_QUEUE_SIZE - 1]);
    EQUAL(p, NULL);
    p = unqueue_matching(&queue, /*max_queue=*/ 1, matcher, &things[MAX_QUEUE_SIZE - 1]);
    EQUAL(p, &things[MAX_QUEUE_SIZE - 1]);
    EQUAL(queue.num_items, MAX_QUEUE_SIZE - 1);
    p = unqueue_matching(&queue, /*max_queue=*/ MAX_QUEUE_SIZE - 2, matcher, &things[0]);
    EQUAL(p, NULL);
    p = unqueue_matching(&queue, /*max_queue=*/ MAX_QUEUE_SIZE - 1, matcher, &things[0]);
    EQUAL(p, &things[0]);
    EQUAL(queue.num_items, MAX_QUEUE_SIZE - 2);

    // fill the queue then remove every other item
    init_queue(&queue);
    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        if (!put_queue(&queue, &things[i]))
        {
            FAIL;
        }
    }
    p = unqueue_matching(&queue, MAX_QUEUE_SIZE, matcher, &thing1);
    EQUAL(p, NULL);
    for (int i = MAX_QUEUE_SIZE - 1; i >= 0; i -= 2)
    {
        p = unqueue_matching(&queue, MAX_QUEUE_SIZE, matcher, &things[i]);
        EQUAL(p, &things[i]);
    }
    EQUAL(queue.num_items, MAX_QUEUE_SIZE / 2);
    p = unqueue_matching(&queue, MAX_QUEUE_SIZE, matcher, &thing1);
    EQUAL(p, NULL);
    for (int i = 0; i < MAX_QUEUE_SIZE; i += 2)
    {
        p = get_queue(&queue);
        EQUAL(p, &things[i]);
    }
    EQUAL(queue.num_items, 0);

    // fill the queue then remove every third item
    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        if (!put_queue(&queue, &things[i]))
        {
            FAIL;
        }
    }
    p = unqueue_matching(&queue, MAX_QUEUE_SIZE, matcher, &thing1);
    EQUAL(p, NULL);
    for (int i = 0; i < MAX_QUEUE_SIZE; i += 3)
    {
        p = unqueue_matching(&queue, MAX_QUEUE_SIZE, matcher, &things[i]);
        EQUAL(p, &things[i]);
    }
    EQUAL(queue.num_items, MAX_QUEUE_SIZE * 2 / 3);
    p = unqueue_matching(&queue, MAX_QUEUE_SIZE, matcher, &thing1);
    EQUAL(p, NULL);
    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        if (i % 3 == 0)
            continue;
        p = get_queue(&queue);
        EQUAL(p, &things[i]);
    }
    EQUAL(queue.num_items, 0);

    if (!pass)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
