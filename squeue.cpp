#include "include/squeue.h"
#include <iostream>

pthread_cond_t has_room = PTHREAD_COND_INITIALIZER;
pthread_cond_t has_req = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
int num_req = 0;

Node* head = NULL;
Node* tail = NULL;  // keep tail pointer for faster enqueue

/**
 * Enter a new client to the waiting queue.
 * If the size of queue reach the max allowed size,
 * keep waiting untile there is room(space).
 * Producer.
 * @param client The new request client to enqueue.
 * @param max_req The maximum allowed queue size.
 */
void enqueue(MySocket* client, int max_req) {
  dthread_mutex_lock(&queue_lock);

  while (num_req == max_req) {
    dthread_cond_wait(&has_room, &queue_lock);
  }

  Node* new_node = new Node(client, NULL);
  if (!tail) {
    head = new_node;
  } 
  tail = new_node;
  num_req++;

  #ifdef _DEBUG_SHOW_STEP_
  std::cout << "enqueue!" << std::endl;
  #endif

  dthread_cond_signal(&has_req);
  dthread_mutex_unlock(&queue_lock);
}

/**
 * Extract the first waiting request client.
 * If the waiting is empty,
 * keep waiting untile it is not.
 * Comsumer.
 * @return The front request client of queue.
 */
MySocket* dequeue() {
  dthread_mutex_lock(&queue_lock);
  while (num_req == 0) {
    dthread_cond_wait(&has_req, &queue_lock);
  }

  while (!head || num_req == 0) {
    dthread_cond_wait(&has_req, &queue_lock);
  }

  MySocket* front = head->client;
  Node* temp = head;
  head = head->next;
  if (!head) {
    tail = NULL;
  }
  delete temp;
  num_req--;
  
  #ifdef _DEBUG_SHOW_STEP_
  std::cout << "dequeue!" << std::endl;
  #endif

  dthread_cond_signal(&has_room);
  dthread_mutex_unlock(&queue_lock);
  return front;
}

