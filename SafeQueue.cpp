#include "include/SafeQueue.h"

#include <iostream>
#include <deque>
#include <vector>

pthread_cond_t has_room = PTHREAD_COND_INITIALIZER;
pthread_cond_t has_req = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

std::deque<MySocket*> waiting_queue;
int buff_size = 1;

void set_buff_size(int max_req) {
  buff_size = max_req;
}

/**
 * Enter a new client to the waiting queue.
 * If the size of queue reach the max allowed size,
 * keep waiting untile there is room(space).
 * Producer.
 * @param client The new request client to enqueue.
 */
void enqueue(MySocket* client) {
  dthread_mutex_lock(&queue_lock);

  while ((int)waiting_queue.size() >= buff_size) {
    dthread_cond_wait(&has_room, &queue_lock);
  }

  waiting_queue.push_back(client);

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
  
  while (waiting_queue.empty()) {
    dthread_cond_wait(&has_req, &queue_lock);
  }

  MySocket* front = waiting_queue.front();
  waiting_queue.pop_front();

  #ifdef _DEBUG_SHOW_STEP_
  std::cout << "dequeue!" << std::endl;
  #endif

  dthread_cond_signal(&has_room);
  dthread_mutex_unlock(&queue_lock);
  return front;
}

