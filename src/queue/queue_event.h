#ifndef QUEUE_EVENT_H_ 
#define QUEUE_EVENT_H_


#include <glib-object.h>

G_BEGIN_DECLS

#define QUEUE_TYPE_QUEUEEVENT QueueEvent__get_type()
G_DECLARE_FINAL_TYPE (QueueEvent, QueueEvent_, QUEUE, QUEUEEVENT, GObject)

struct _QueueEvent
{
  GObject parent_instance;
};


struct _QueueEventClass
{
  GObjectClass parent_class;
};

QueueEvent* QueueEvent__new(); 
void * QueueEvent__get_scope(QueueEvent * evt);
void QueueEvent__cancel(QueueEvent * self);
int QueueEvent__is_cancelled(QueueEvent * self);
void QueueEvent__invoke(QueueEvent * self);
void QueueEvent__cleanup(QueueEvent * self);

G_END_DECLS

#endif