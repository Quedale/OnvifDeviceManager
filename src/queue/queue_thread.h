#ifndef QUEUE_THREAD_H_ 
#define QUEUE_THREAD_H_

#include <stdlib.h>

typedef struct _QueueThread QueueThread;
#include "event_queue.h"

QueueThread * QueueThread__create(EventQueue* queue); 
void QueueThread__init(QueueThread * self, EventQueue* queue);
void QueueThread__cancel(QueueThread* self);
int QueueThread__is_cancelled(QueueThread* self);
EventQueue* QueueThread__get_queue(QueueThread* self);

#endif