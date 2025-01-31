#ifndef QUEUE_THREAD_H_ 
#define QUEUE_THREAD_H_

#include <stdlib.h>
#include <glib-object.h>

typedef struct _QueueThread QueueThread;

#include "event_queue.h"
#include "queue_event.h"

G_BEGIN_DECLS

typedef enum {
  QUEUETHREAD_STARTED                = 0,
  QUEUETHREAD_FINISHED                = 1
} QueueThreadState;

#ifndef g_enum_to_nick
#define g_enum_to_nick(type,val) (g_enum_get_value(g_type_class_ref (type),val)->value_nick)
#endif

GType QueueThreadState__get_type (void) G_GNUC_CONST;
#define QUEUE_TYPE_THREADSTATE (QueueThreadState__get_type())

#define QUEUE_TYPE_THREAD QueueThread__get_type()
G_DECLARE_FINAL_TYPE (QueueThread, QueueThread_, QUEUE, THREAD, GObject)

struct _QueueThread {
  GObject parent_instance;
};


struct _QueueThreadClass {
  GObjectClass parent_class;
};

QueueThread * QueueThread__new(EventQueue* queue); 
void QueueThread__terminate(QueueThread* self);
int QueueThread__is_terminated(QueueThread* self);
void QueueThread__start(QueueThread * self);

//Thread-local function returning the current context pointers
//Designed to be used within background events
EventQueue * EventQueue__get_current();
QueueEvent * QueueEvent__get_current();
QueueThread * QueueThread__get_current();

G_END_DECLS

#endif