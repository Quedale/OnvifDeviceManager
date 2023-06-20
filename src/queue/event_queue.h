#ifndef EVENT_QUEUE_H_ 
#define EVENT_QUEUE_H_

#include <stdlib.h>

typedef enum {
  EVENTQUEUE_DISPATCHING               = 0,
  EVENTQUEUE_DISPATCHED             = 1,
  EVENTQUEUE_STARTED                = 2
  //TODO Handle more types
} EventQueueType;

typedef struct _EventQueue EventQueue;

typedef struct {
  void * user_data;
  void (*callback)();
  // int * empty;
} QueueEvent;

EventQueue* EventQueue__create(void (*queue_event_cb)(EventQueue * queue, EventQueueType type,void * user_data),void * user_data); 

void EventQueue__destroy(EventQueue* onvifDeviceList);
void EventQueue__insert(EventQueue* queue, void (*callback)(), void * user_data);
QueueEvent EventQueue__pop(EventQueue* self);
void EventQueue__start(EventQueue* self);
int EventQueue__get_running_event_count(EventQueue * self);
int EventQueue__get_pending_event_count(EventQueue * self);
int EventQueue__get_thread_count(EventQueue * self);

#endif