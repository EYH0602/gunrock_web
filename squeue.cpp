#include "squeue.h"

pthread_cond_t queue_cond;
pthread_mutex_t queue_lock;

void enqueue(Node* queue, Node* node) {
  dthread_mutex_lock(&queue_lock);

  // find tail
  Node* ptr;
  for (ptr = queue; ptr->next; ptr = ptr->next);
  // add element to tail
  ptr->next = node;

  dthread_cond_signal(&queue_cond);
  dthread_mutex_unlock(&queue_lock);
}

Node* dequeue(Node* queue) {
  dthread_mutex_lock(&queue_lock);

  while (is_empty(queue)) {
    dthread_cond_wait(&queue_cond, &queue_lock);
  }

  // remove from queue
  Node* item = queue;
  queue = item->next;
  item->next = NULL;

  dthread_mutex_unlock(&queue_lock);
  return item; 
}

bool is_empty(Node* queue) {
  return queue->next == NULL;
}

