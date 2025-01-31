#ifndef EVENT_QUEUE_H_ 
#define EVENT_QUEUE_H_

#include <stdlib.h>
#include <glib-object.h>

typedef struct _EventQueue EventQueue;

#include "queue_thread.h"
#include "queue_event.h"
#include "portable_thread.h"

G_BEGIN_DECLS

typedef enum {
  EVENTQUEUE_DISPATCHING            = 0,
  EVENTQUEUE_DISPATCHED             = 1,
  EVENTQUEUE_CANCELLED              = 2,
  EVENTQUEUE_ADDED                  = 3,
  EVENTQUEUE_STARTED                = 4,
  EVENTQUEUE_FINISHED                = 5
} QueueEventType;

#ifndef g_enum_to_nick
#define g_enum_to_nick(type,val) (g_enum_get_value(g_type_class_ref (type),val)->value_nick)
#endif

GType QueueEventType__get_type (void) G_GNUC_CONST;
#define QUEUE_TYPE_EVENTTYPE (QueueEventType__get_type())

#define QUEUE_TYPE_EVENTQUEUE EventQueue__get_type()
G_DECLARE_FINAL_TYPE (EventQueue, EventQueue_, QUEUE, EVENTQUEUE, GObject)

struct _EventQueue {
  GObject parent_instance;
};


struct _EventQueueClass {
  GObjectClass parent_class;
};

EventQueue* EventQueue__new(); 

QueueEvent * EventQueue__insert(EventQueue* queue, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), gpointer user_data, void (*cleanup_cb)(QueueEvent * qevt, int cancelled, void * user_data));
QueueEvent * EventQueue__insert_plain(EventQueue* self, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), void * user_data, void (*cleanup_cb)(QueueEvent * qevt, int cancelled, void * user_data));
QueueEvent * EventQueue__pop(EventQueue* self);
void EventQueue__start(EventQueue* self);
void EventQueue__stop(EventQueue* self, int nthread);
int EventQueue__get_thread_count(EventQueue * self);
void EventQueue__cancel_scopes(EventQueue * self, void ** scopes, int count);
void EventQueue__wait_condition(EventQueue * self, P_MUTEX_TYPE lock);

G_END_DECLS

#endif