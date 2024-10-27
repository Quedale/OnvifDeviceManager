#ifndef QUEUE_THREAD_H_ 
#define QUEUE_THREAD_H_

#include <stdlib.h>
#include <glib-object.h>

typedef struct _QueueThread QueueThread;

#include "event_queue.h"
#include "queue_event.h"

G_BEGIN_DECLS

typedef enum {
  EVENTQUEUE_DISPATCHING            = 0,
  EVENTQUEUE_DISPATCHED             = 1,
  EVENTQUEUE_CANCELLED              = 2,
  EVENTQUEUE_STARTED                = 3,
  EVENTQUEUE_FINISHED                = 4
} QueueEventType;

#ifndef g_enum_to_nick
#define g_enum_to_nick(type,val) (g_enum_get_value(g_type_class_ref (type),val)->value_nick)
#endif

GType QueueEventType__get_type (void) G_GNUC_CONST;
#define QUEUE_TYPE_EVENTTYPE (QueueEventType__get_type())


#define QUEUE_TYPE_THREAD QueueThread__get_type()
G_DECLARE_FINAL_TYPE (QueueThread, QueueThread_, QUEUE, THREAD, GObject)


// typedef struct _QueueThread QueueThread;
struct _QueueThread
{
  GObject parent_instance;
};


struct _QueueThreadClass
{
  GObjectClass parent_class;
};


QueueThread * QueueThread__new(EventQueue* queue); 
void QueueThread__cancel(QueueThread* self);
int QueueThread__is_cancelled(QueueThread* self);
void QueueThread__start(QueueThread * self);

//Thread-local function returning the current context pointers
//Designed to be used within background events
QueueEvent * QueueEvent__get_current();
QueueThread * QueueThread__get_current();

G_END_DECLS

#endif