#ifndef EVENT_QUEUE_H_ 
#define EVENT_QUEUE_H_

#include <stdlib.h>

typedef struct {
    void * user_data;
    void (*callback)();
    // int * empty;
} QueueEvent;


typedef struct {
  int event_count;
  pthread_t tid;
  pthread_cond_t * sleep_cond;
  pthread_mutex_t * sleep_lock;
  pthread_cond_t * pop_cond;
  pthread_mutex_t * pop_lock;
  QueueEvent *events;
} EventQueue;

__attribute__ ((visibility("default"))) 
extern EventQueue* EventQueue__create(); 

__attribute__ ((visibility("default"))) 
extern void EventQueue__destroy(EventQueue* onvifDeviceList);
__attribute__ ((visibility("default"))) 
extern void EventQueue__insert(EventQueue* queue, void (*callback)(), void * user_data);
__attribute__ ((visibility("default"))) 
extern QueueEvent EventQueue__pop(EventQueue* self);
__attribute__ ((visibility("default"))) 
extern void EventQueue__start(EventQueue* self);

#endif