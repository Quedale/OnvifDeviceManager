#include "queue_thread.h"
#include "queue_event.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "clogger.h"
#include "threads.h"

enum {
    STATE_CHANGED,
    LAST_SIGNAL
};

enum
{
  PROP_QUEUE = 1,
  N_PROPERTIES
};

typedef struct {
    P_THREAD_TYPE pthread;
    int started;
    EventQueue * queue;
    P_MUTEX_TYPE sleep_lock;
    P_MUTEX_TYPE cancel_lock;
    int cancelled;
} QueueThreadPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(QueueThread, QueueThread_, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

GType
QueueEventType__get_type (void)
{
        static gsize g_define_type_id__volatile = 0;

        if (g_once_init_enter(&g_define_type_id__volatile)) {
                static const GEnumValue values[] = {
                        { EVENTQUEUE_DISPATCHING,   "EVENTQUEUE_DISPATCHING",   "Dispatching"},
                        { EVENTQUEUE_DISPATCHED,    "EVENTQUEUE_DISPATCHED",    "Dispatched"},
                        { EVENTQUEUE_CANCELLED,     "EVENTQUEUE_CANCELLED",     "Cancelled"},
                        { EVENTQUEUE_ADDED,         "EVENTQUEUE_ADDED",         "Added"},
                        { EVENTQUEUE_STARTED,       "EVENTQUEUE_STARTED",       "Started"},
                        { EVENTQUEUE_FINISHED,      "EVENTQUEUE_FINISHED",      "Finished"},
                        { 0,                        NULL,                       NULL}
                };
                GType g_define_type_id = g_enum_register_static(g_intern_static_string("QueueEventType"), values);
                g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
        }

        return g_define_type_id__volatile;
}

void QueueThread__dispose(GObject * obj){
    QueueThreadPrivate *priv = QueueThread__get_instance_private (QUEUE_THREAD(obj));
    P_MUTEX_CLEANUP(priv->sleep_lock);
    P_MUTEX_CLEANUP(priv->cancel_lock);
    priv->started = 0;
    priv->cancelled = 0;
    G_OBJECT_CLASS (QueueThread__parent_class)->dispose (obj);
}

static _Thread_local QueueEvent * event_queue;
static _Thread_local QueueThread * queue_thread;
QueueEvent * QueueEvent__get_current(){
    return event_queue;
}

QueueThread * QueueThread__get_current(){
    return queue_thread;
}

void * priv_QueueThread_call(void * data){
    queue_thread = (QueueThread*) data;
    event_queue = NULL;
    QueueThreadPrivate *priv = QueueThread__get_instance_private (queue_thread);

    g_signal_emit (queue_thread, signals[STATE_CHANGED], 0, EVENTQUEUE_STARTED /* details */);
    while (1){
        if(QueueThread__is_cancelled(queue_thread)){
            goto exit;
        }
        if(EventQueue__get_pending_event_count(priv->queue) <= 0){
            P_MUTEX_LOCK(priv->sleep_lock);
            if(QueueThread__is_cancelled(queue_thread)){
                P_MUTEX_UNLOCK(priv->sleep_lock);
                goto exit;
            }
            EventQueue__wait_condition(priv->queue,priv->sleep_lock);
            P_MUTEX_UNLOCK(priv->sleep_lock);
        }
        if(QueueThread__is_cancelled(queue_thread)){
            goto exit;
        }

        event_queue = EventQueue__pop(priv->queue);
        if(!event_queue){
            continue;
        }
        g_signal_emit (queue_thread, signals[STATE_CHANGED], 0, EVENTQUEUE_DISPATCHING /* details */);
        
        QueueEvent__invoke(event_queue);
        if(QueueEvent__is_cancelled(event_queue)){
            g_signal_emit (queue_thread, signals[STATE_CHANGED], 0, EVENTQUEUE_CANCELLED /* details */);
        } else {
            QueueEvent__finish(event_queue);
            g_signal_emit (queue_thread, signals[STATE_CHANGED], 0, EVENTQUEUE_DISPATCHED /* details */);
        }
        QueueEvent__cleanup(event_queue);
        g_object_unref(event_queue);
    }

exit:
    g_signal_emit (queue_thread, signals[STATE_CHANGED], 0, EVENTQUEUE_FINISHED /* details */);
    g_object_unref(queue_thread);
    P_THREAD_EXIT;
}


static void
QueueThread__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    QueueThread * thread = QUEUE_THREAD (object);
    QueueThreadPrivate *priv = QueueThread__get_instance_private (thread);
    switch (prop_id){
        case PROP_QUEUE:
            priv->queue = g_value_get_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
QueueThread__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    QueueThread *thread = QUEUE_THREAD (object);
    QueueThreadPrivate *priv = QueueThread__get_instance_private (thread);
    switch (prop_id){
        case PROP_QUEUE:
            g_value_set_object (value, priv->queue);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
QueueThread__class_init (QueueThreadClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = QueueThread__dispose;
    object_class->set_property = QueueThread__set_property;
    object_class->get_property = QueueThread__get_property;

    GType params[1];
    params[0] = QUEUE_TYPE_EVENTTYPE | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[STATE_CHANGED] =
        g_signal_newv ("state-changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);


    obj_properties[PROP_QUEUE] =
        g_param_spec_object ("queue",
                            "EventQueue",
                            "Pointer to EventQueue parent.",
                            QUEUE_TYPE_EVENTQUEUE  /* default value */,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

void QueueThread__init(QueueThread * self){
    QueueThreadPrivate *priv = QueueThread__get_instance_private (self);
    priv->queue = NULL;
    
    P_MUTEX_SETUP(priv->sleep_lock);
    P_MUTEX_SETUP(priv->cancel_lock);
    priv->started = 0;
    priv->cancelled = 0;
}

void QueueThread__start(QueueThread * self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_THREAD (self));

    QueueThreadPrivate *priv = QueueThread__get_instance_private (self);
    g_return_if_fail (priv->started == 0); 
    g_object_ref(self);
    P_THREAD_CREATE(priv->pthread, priv_QueueThread_call, self);
    P_THREAD_DETACH(priv->pthread);
    priv->started = 1;
}

QueueThread * QueueThread__new(EventQueue* queue){
    return g_object_new (QUEUE_TYPE_THREAD, "queue", queue, NULL);
}

void QueueThread__cancel(QueueThread* self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_THREAD (self));
    QueueThreadPrivate *priv = QueueThread__get_instance_private (self);
    P_MUTEX_LOCK(priv->cancel_lock);
    priv->cancelled = 1;
    P_MUTEX_UNLOCK(priv->cancel_lock);
}

int QueueThread__is_cancelled(QueueThread* self){
    g_return_val_if_fail (self != NULL,FALSE);
    g_return_val_if_fail (QUEUE_IS_THREAD (self),FALSE);
    QueueThreadPrivate *priv = QueueThread__get_instance_private (self);

    int ret;
    P_MUTEX_LOCK(priv->cancel_lock);
    ret = priv->cancelled;
    P_MUTEX_UNLOCK(priv->cancel_lock);
    return ret;
}