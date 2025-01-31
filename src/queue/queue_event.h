#ifndef QUEUE_EVENT_H_ 
#define QUEUE_EVENT_H_


#include <glib-object.h>

G_BEGIN_DECLS

#define QUEUE_TYPE_QUEUEEVENT QueueEvent__get_type()
G_DECLARE_FINAL_TYPE (QueueEvent, QueueEvent_, QUEUE, QUEUEEVENT, GObject)

typedef void (*QueueEventCallback) (QueueEvent  *self, void * user_data);
#define QUEUEEVENT_CALLBACK_FUNC(f) ((QueueEventCallback) (void (*)(void)) (f))

typedef void (*QueueEventCleanupCallback) (QueueEvent  *self, int cancelled, void * user_data);
#define QUEUEEVENT_CLEANUP_FUNC(f) ((QueueEventCleanupCallback) (void (*)(void)) (f))

struct _QueueEvent {
  GObject parent_instance;
};

struct _QueueEventClass {
  GObjectClass parent_class;
};

typedef enum {
  QUEUEEVENT_DISPATCHED             = 0,
  QUEUEEVENT_CANCELLED              = 1
} QueueEventState;

#ifndef g_enum_to_nick
#define g_enum_to_nick(type,val) (g_enum_get_value(g_type_class_ref (type),val)->value_nick)
#endif

GType QueueEventState__get_type (void) G_GNUC_CONST;
#define QUEUE_TYPE_EVENTSTATE (QueueEventState__get_type())

QueueEvent* QueueEvent__new(void * scope, QueueEventCallback callback, QueueEventCleanupCallback cleanup_cb, void * user_data, int managed);
void * QueueEvent__get_scope(QueueEvent * evt);
void QueueEvent__cancel(QueueEvent * self);
int QueueEvent__is_cancelled(QueueEvent * self);
int QueueEvent__is_finished(QueueEvent * self);
void QueueEvent__invoke(QueueEvent * self);

G_END_DECLS

#endif