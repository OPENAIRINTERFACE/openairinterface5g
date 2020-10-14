#include "queue.h"
#include "common/utils/LOG/log.h"
#include <string.h>

void init_queue(queue_t *q) {
  memset(q, 0, sizeof(*q));
  pthread_mutex_init(&q->mutex, NULL);
}

bool put_queue(queue_t *q, void *item) {

  if (pthread_mutex_lock(&q->mutex) != 0) {
    LOG_E(PHY, "put_queue mutex_lock failed\n");
    return false;
  }

  bool queued;
  if (q->num_items >= MAX_QUEUE_SIZE) {
    LOG_E(PHY, "Queue is full in put_queue\n");
    queued = false;
  } else {
    q->items[q->write_index] = item;
    q->write_index = (q->write_index + 1) % MAX_QUEUE_SIZE;
    q->num_items++;
    queued = true;
  }

  pthread_mutex_unlock(&q->mutex);
  return queued;
}

void *get_queue(queue_t *q) {

  void *item = NULL;
  if (pthread_mutex_lock(&q->mutex) != 0) {
    LOG_E(PHY, "get_queue mutex_lock failed\n");
    return NULL;
  }

  if (q->num_items > 0) {
    item = q->items[q->read_index];
    q->read_index = (q->read_index + 1) % MAX_QUEUE_SIZE;
    q->num_items--;
  }

  pthread_mutex_unlock(&q->mutex);
  return item;
}

void *unqueue(queue_t *q)
{
  void *item = NULL;
  if (pthread_mutex_lock(&q->mutex) != 0) {
    LOG_E(PHY, "remove_from_back_of_queue mutex_lock failed\n");
    return NULL;
  }

  if (q->num_items > 0) {
    q->write_index = (q->write_index + MAX_QUEUE_SIZE - 1) % MAX_QUEUE_SIZE;
    item = q->items[q->write_index];
    q->num_items--;
  }

  pthread_mutex_unlock(&q->mutex);
  return item;
}
