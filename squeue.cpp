#include "include/squeue.h"
#include <iostream>

pthread_cond_t has_room = PTHREAD_COND_INITIALIZER;
pthread_cond_t has_req = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
int num_req = 0;


/* producer */
void enqueue(Node* queue, Node* node, int max_req) {
  dthread_mutex_lock(&queue_lock);

  while (num_req == max_req) {
    dthread_cond_wait(&has_room, &queue_lock);
  }

  // find tail
  Node* ptr = queue;
  while (ptr != NULL) {
    ptr = ptr->next;
  }
  // add element to tail
  ptr = node;
  num_req++;

  dthread_cond_signal(&has_req);
  dthread_mutex_unlock(&queue_lock);
}

/* consumer */
Node* dequeue(Node* queue) {
  dthread_mutex_lock(&queue_lock);
  while (num_req == 0) {
    dthread_cond_wait(&has_req, &queue_lock);
  }

  // remove from queue
  Node* item = queue;
  queue = item->next;
  item->next = NULL;
  num_req--;

  dthread_cond_signal(&has_room);
  dthread_mutex_unlock(&queue_lock);
  return item; 
}

