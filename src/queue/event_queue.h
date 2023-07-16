#ifndef EVENT_QUEUE_H_ 
#define EVENT_QUEUE_H_

#include <stdlib.h>

typedef struct _EventQueue EventQueue;

#include "queue_thread.h"
#include "queue_event.h"

typedef enum {
  EVENTQUEUE_DISPATCHING               = 0,
  EVENTQUEUE_DISPATCHED             = 1,
  EVENTQUEUE_STARTED                = 2
  //TODO Handle more types
} EventQueueType;

EventQueue* EventQueue__create(void (*queue_event_cb)(QueueThread * thread, EventQueueType type,void * user_data),void * user_data); 

void EventQueue__insert(EventQueue* queue, void (*callback)(), void * user_data);
QueueEvent * EventQueue__pop(EventQueue* self);
void EventQueue__start(EventQueue* self);
void EventQueue__stop(EventQueue* self, int nthread);
int EventQueue__get_running_event_count(EventQueue * self);
int EventQueue__get_pending_event_count(EventQueue * self);
int EventQueue__get_thread_count(EventQueue * self);
void EventQueue__wait_condition(EventQueue * self, pthread_mutex_t * lock);
void EventQueue_notify_dispatching(EventQueue * self, QueueThread * thread);
void EventQueue_notify_dispatched(EventQueue * self, QueueThread * thread);
void EventQueue__remove_thread(EventQueue* self, QueueThread * qt);

#endif