#ifndef EVENT_QUEUE_H_ 
#define EVENT_QUEUE_H_

#include <stdlib.h>

typedef struct _EventQueue EventQueue;

#include "queue_thread.h"
#include "queue_event.h"
#include "portable_thread.h"

typedef enum {
  EVENTQUEUE_DISPATCHING            = 0,
  EVENTQUEUE_DISPATCHED             = 1,
  EVENTQUEUE_CANCELLED              = 2,
  EVENTQUEUE_STARTED                = 3
  //TODO Handle more types
} EventQueueType;

const char * EventQueueType__toString(EventQueueType type);

EventQueue* EventQueue__create(void (*queue_event_cb)(EventQueue * self, EventQueueType type,void * user_data),void * user_data); 

void EventQueue__insert(EventQueue* queue, void * scope, void (*callback)(void * user_data), void * user_data);
QueueEvent * EventQueue__pop(EventQueue* self);
void EventQueue__clear(EventQueue * self);
void EventQueue__start(EventQueue* self);
void EventQueue__stop(EventQueue* self, int nthread);
int EventQueue__get_running_event_count(EventQueue * self);
int EventQueue__get_pending_event_count(EventQueue * self);
int EventQueue__get_thread_count(EventQueue * self);
void EventQueue__wait_condition(EventQueue * self, P_MUTEX_TYPE lock);
void EventQueue_notify(EventQueue * self, EventQueueType type);
void EventQueue__remove_thread(EventQueue* self, QueueThread * qt);
void EventQueue__cancel_scopes(EventQueue * self, void ** scopes, int count);

#endif