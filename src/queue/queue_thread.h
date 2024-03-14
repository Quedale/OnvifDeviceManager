#ifndef QUEUE_THREAD_H_ 
#define QUEUE_THREAD_H_

#include <stdlib.h>

typedef struct _QueueThread QueueThread;
#include "event_queue.h"
#include "queue_event.h"

QueueThread * QueueThread__create(EventQueue* queue); 
void QueueThread__init(QueueThread * self, EventQueue* queue);
void QueueThread__cancel(QueueThread* self);
int QueueThread__is_cancelled(QueueThread* self);
EventQueue* QueueThread__get_queue(QueueThread* self);

//Thread-local function returning the current context pointers
//Designed to be used within background events
QueueEvent * QueueEvent__get_current();
QueueThread * QueueThread__get_current();

#endif