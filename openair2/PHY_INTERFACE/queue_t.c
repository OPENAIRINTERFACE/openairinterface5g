#include "queue_t.h"
#include <string.h>
#include <assert.h>

#ifdef UNITTEST
#include <stdio.h>
#define LOG_ERROR(MSG) printf(MSG "\n")
#else
#include "common/utils/LOG/log.h"
#define LOG_ERROR(MSG) LOG_E(PHY, MSG "\n")
#endif

void init_queue(queue_t *q)
{
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->mutex, NULL);
}

void reset_queue(queue_t *q)
{
  void *p;
  while ((p = get_queue(q)) != NULL)
  {
    free(p);
  }
}

void *put_queue_replace(queue_t *q, void *item)
{
    assert(item != NULL);
    if (pthread_mutex_lock(&q->mutex) != 0)
    {
        LOG_ERROR("put_queue: mutex_lock failed");
        return false;
    }

    void *evicted = NULL;
    if (q->num_items >= MAX_QUEUE_SIZE)
    {
        evicted = q->items[q->read_index];
        assert(evicted != NULL);
        q->items[q->read_index] = NULL;
        q->read_index = (q->read_index + 1) % MAX_QUEUE_SIZE;
        q->num_items--;
    }
    assert(q->items[q->write_index] == NULL);
    q->items[q->write_index] = item;
    q->write_index = (q->write_index + 1) % MAX_QUEUE_SIZE;
    q->num_items++;

    pthread_mutex_unlock(&q->mutex);
    return evicted;
}


bool put_queue(queue_t *q, void *item)
{
    assert(item != NULL);
    if (pthread_mutex_lock(&q->mutex) != 0)
    {
        LOG_ERROR("put_queue: mutex_lock failed");
        return false;
    }

    bool queued;
    if (q->num_items >= MAX_QUEUE_SIZE)
    {
        LOG_ERROR("put_queue: queue is full");
        queued = false;
    }
    else
    {
        assert(q->items[q->write_index] == NULL);
        q->items[q->write_index] = item;
        q->write_index = (q->write_index + 1) % MAX_QUEUE_SIZE;
        q->num_items++;
        queued = true;
    }

    pthread_mutex_unlock(&q->mutex);
    return queued;
}

void *get_queue(queue_t *q)
{
    void *item = NULL;
    if (pthread_mutex_lock(&q->mutex) != 0)
    {
        LOG_ERROR("get_queue: mutex_lock failed");
        return NULL;
    }

    if (q->num_items > 0)
    {
        item = q->items[q->read_index];
        assert(item != NULL);
        q->items[q->read_index] = NULL;
        q->read_index = (q->read_index + 1) % MAX_QUEUE_SIZE;
        q->num_items--;
    }

    pthread_mutex_unlock(&q->mutex);
    return item;
}

bool requeue(queue_t *q, void *item)
{
    assert(item != NULL);
    if (pthread_mutex_lock(&q->mutex) != 0)
    {
        LOG_ERROR("requeue: mutex_lock failed");
        return false;
    }

    bool queued;
    if (q->num_items >= MAX_QUEUE_SIZE)
    {
        LOG_ERROR("requeue: queue is full");
        queued = false;
    }
    else
    {
        q->read_index = (q->read_index + MAX_QUEUE_SIZE - 1) % MAX_QUEUE_SIZE;
        assert(q->items[q->read_index] == NULL);
        q->items[q->read_index] = item;
        q->num_items++;
        queued = true;
    }

    pthread_mutex_unlock(&q->mutex);
    return queued;
}

void *unqueue(queue_t *q)
{
    void *item = NULL;
    if (pthread_mutex_lock(&q->mutex) != 0) {
        LOG_ERROR("unqueue: mutex_lock failed");
        return NULL;
    }

    if (q->num_items > 0) {
        q->write_index = (q->write_index + MAX_QUEUE_SIZE - 1) % MAX_QUEUE_SIZE;
        item = q->items[q->write_index];
        q->items[q->write_index] = NULL;
        q->num_items--;
    }

    pthread_mutex_unlock(&q->mutex);
    return item;
}

void *unqueue_matching(queue_t *q, size_t max_depth, queue_matcher_t *matcher, void *wanted)
{
    if (pthread_mutex_lock(&q->mutex) != 0)
    {
        LOG_ERROR("unqueue_matching: mutex_lock failed");
        return NULL;
    }

    void *item = NULL;
    size_t endi = q->write_index;
    for (size_t i = 0; i < q->num_items; i++)
    {
        if (max_depth == 0)
        {
            break;
        }
        --max_depth;

        endi = (endi + MAX_QUEUE_SIZE - 1) % MAX_QUEUE_SIZE;
        void *candidate = q->items[endi];
        if (matcher(wanted, candidate))
        {
            item = candidate;
            // delete item from the queue and move other items down
            for (;;)
            {
                size_t j = (endi + 1) % MAX_QUEUE_SIZE;
                if (j == q->write_index)
                {
                    q->items[endi] = NULL;
                    q->write_index = endi;
                    q->num_items--;
                    break;
                }
                q->items[endi] = q->items[j];
                endi = j;
            }
            break;
        }
    }

    pthread_mutex_unlock(&q->mutex);
    return item;
}
