#ifndef QUEUE_EVENT_H_ 
#define QUEUE_EVENT_H_

typedef struct _QueueEvent  QueueEvent;

#include "queue_thread.h"

QueueEvent * QueueEvent__create(void * scope, void (*callback)(void * user_data), void * user_data, void (*cleanup_cb)(int cancelled, void * user_data), int managed); 
void QueueEvent__init(QueueEvent * self,void * scope, void (*callback)(void * user_data), void * user_data, void (*cleanup_cb)(int cancelled, void * user_data), int managed);
void * QueueEvent__get_scope(QueueEvent * evt);
void QueueEvent__cancel(QueueEvent * self);
int QueueEvent__is_cancelled(QueueEvent * self);
void QueueEvent__invoke(QueueEvent * self);
void QueueEvent__cleanup(QueueEvent * self);

#endif