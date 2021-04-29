#ifndef _SQUEUE_H_
#define _SQUEUE_H_

#include "MySocket.h"
#include "dthread.h"


struct Node {
  MySocket* client;
  Node* next;

  Node(MySocket* client_arg) {
    this->client = client_arg;
    this->next = NULL;
  }

  Node(MySocket* client_arg, Node* next_arg) {
    this->client = client_arg;
    this->next = next_arg;
  }
};

void enqueue(Node* queue, Node* node);
Node* dequeue(Node* queue);
bool is_empty(Node* queue);


#endif