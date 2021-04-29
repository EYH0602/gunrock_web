#include "squeue.h"
#include <iostream>

pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

void enqueue(Node* queue, Node* node) {
  dthread_mutex_lock(&queue_lock);

  // find tail
  Node* ptr = queue;
  while (ptr != NULL) {
    ptr = ptr->next;
  }
  // add element to tail
  ptr = node;

  dthread_cond_signal(&queue_cond);
  dthread_mutex_unlock(&queue_lock);
}

Node* dequeue(Node* queue) {
  dthread_mutex_lock(&queue_lock);
  while (!queue) {
    dthread_cond_wait(&queue_cond, &queue_lock);
  }

  // remove from queue
  Node* item = queue;
  queue = item->next;
  item->next = NULL;

  dthread_mutex_unlock(&queue_lock);
  return item; 
}

