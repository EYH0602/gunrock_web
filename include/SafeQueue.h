#ifndef _SAFEQUEUE_H_
#define _SAFEQUEUE_H_

#include "MySocket.h"
#include "dthread.h"

void set_buff_size(int max_req);

void enqueue(MySocket* client);
MySocket* dequeue();


#endif