#ifndef QUEUE_EVENT_H_ 
#define QUEUE_EVENT_H_

typedef struct _QueueEvent  QueueEvent;

#include "queue_thread.h"

typedef void (*QUEUE_CALLBACK)(void * user_data);

QueueEvent * QueueEvent__create(void * scope, void (*callback)(void * user_data), void * user_data); 
void QueueEvent__init(QueueEvent * self,void * scope, void (*callback)(void * user_data), void * user_data);
QUEUE_CALLBACK QueueEvent__get_callback(QueueEvent * self);
void * QueueEvent__get_userdata(QueueEvent * self);
void * QueueEvent__get_scope(QueueEvent * evt);
void QueueEvent__cancel(QueueEvent * self);
int QueueEvent__is_cancelled(QueueEvent * self);

#endif