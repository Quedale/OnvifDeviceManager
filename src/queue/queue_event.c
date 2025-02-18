#include "queue_event.h"
#include "portable_thread.h"
#include <stdlib.h>
#include "clogger.h"

enum {
    SIGNAL_STATE_CHANGED,
    LAST_SIGNAL
};

enum {
  PROP_MANAGED = 1,
  PROP_SCOPE = 2,
  PROP_USERDATA = 3,
  N_PROPERTIES
};

typedef struct {
    int managed;
    int cancelled;
    int finished;
    P_MUTEX_TYPE prop_lock;
    void * scope;
    QueueEventCallback callback;
    QueueEventCleanupCallback cleanup_cb;

    void * user_data;
} QueueEventPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(QueueEvent, QueueEvent_, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

GType
QueueEventState__get_type (void){
        static gsize g_define_type_id__volatile = 0;

        if (g_once_init_enter(&g_define_type_id__volatile)) {
                static const GEnumValue values[] = {
                        { QUEUEEVENT_DISPATCHED,    "QUEUEEVENT_DISPATCHED",    "Dispatched"},
                        { QUEUEEVENT_CANCELLED,     "QUEUEEVENT_CANCELLED",     "Cancelled"},
                        { 0,                        NULL,                       NULL}
                };
                GType g_define_type_id = g_enum_register_static(g_intern_static_string("QueueEventState"), values);
                g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
        }

        return g_define_type_id__volatile;
}

static void
QueueEvent__dispose (GObject *object){
    QueueEvent * self = QUEUE_QUEUEEVENT(object);
    QueueEventPrivate *priv = QueueEvent__get_instance_private (self);

    if(priv->cleanup_cb){
        priv->cleanup_cb(self, priv->cancelled, priv->user_data);
        priv->cleanup_cb = NULL;
    }

    if(priv->managed){
        g_object_unref(G_OBJECT(priv->user_data));
        priv->managed = FALSE;
    }

    P_MUTEX_CLEANUP(priv->prop_lock);

    G_OBJECT_CLASS (QueueEvent__parent_class)->dispose (object);
}

static void
QueueEvent__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    QueueEvent * thread = QUEUE_QUEUEEVENT (object);
    QueueEventPrivate *priv = QueueEvent__get_instance_private (thread);
    switch (prop_id){
        case PROP_MANAGED:
            priv->managed = g_value_get_boolean (value);
            break;
        case PROP_SCOPE:
            priv->scope = g_value_get_pointer (value);
            break;
        case PROP_USERDATA:
            priv->user_data = g_value_get_pointer (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
QueueEvent__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    QueueEvent *thread = QUEUE_QUEUEEVENT (object);
    QueueEventPrivate *priv = QueueEvent__get_instance_private (thread);
    switch (prop_id){
        case PROP_MANAGED:
            g_value_set_boolean (value, priv->managed);
            break;
        case PROP_SCOPE:
            g_value_set_pointer (value, priv->scope);
            break;
        case PROP_USERDATA:
            g_value_set_pointer (value, priv->user_data);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
QueueEvent__class_init (QueueEventClass * klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = QueueEvent__dispose;
    object_class->set_property = QueueEvent__set_property;
    object_class->get_property = QueueEvent__get_property;

    GType params[1];
    params[0] = QUEUE_TYPE_EVENTSTATE | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[SIGNAL_STATE_CHANGED] =
        g_signal_newv ("state-changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params);

    obj_properties[PROP_MANAGED] =
        g_param_spec_boolean ("managed",
                            "Managed Event",
                            "True if the event is managed",
                            FALSE,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    obj_properties[PROP_SCOPE] = 
        g_param_spec_pointer ("scope", 
                            "Event cancellation scope", 
                            "Pointer used to group multiple event to determine a cancellation radius.",
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_USERDATA] = 
        g_param_spec_pointer ("userdata", 
                            "Pointer to user provided data", 
                            "Pointer to user provided data used in callbacks",
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

static void 
QueueEvent__init(QueueEvent * self){
    QueueEventPrivate *priv = QueueEvent__get_instance_private (self);
    P_MUTEX_SETUP(priv->prop_lock);
    priv->cancelled = 0;
    priv->finished = 0;
}

QueueEvent* 
QueueEvent__new(void * scope, QueueEventCallback callback, QueueEventCleanupCallback cleanup_cb, void * user_data, int managed){
    QueueEvent * self = g_object_new (QUEUE_TYPE_QUEUEEVENT,
                        "scope", scope,
                        "userdata", user_data,
                        "managed",managed,
                        NULL);
    QueueEventPrivate *priv = QueueEvent__get_instance_private (self);
    priv->callback = callback;
    priv->cleanup_cb = cleanup_cb;
    return self;
}

void 
QueueEvent__cancel(QueueEvent * self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_QUEUEEVENT (self));
    QueueEventPrivate *priv = QueueEvent__get_instance_private (self);

    g_return_if_fail (priv->cancelled == 0);
    P_MUTEX_LOCK(priv->prop_lock);
    priv->cancelled = 1;
    P_MUTEX_UNLOCK(priv->prop_lock);
    g_signal_emit (self, signals[SIGNAL_STATE_CHANGED], 0, QUEUEEVENT_CANCELLED);
}

int 
QueueEvent__is_cancelled(QueueEvent * self){
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (QUEUE_IS_QUEUEEVENT (self), FALSE);
    QueueEventPrivate *priv = QueueEvent__get_instance_private (self);

    int ret;
    P_MUTEX_LOCK(priv->prop_lock);
    ret = priv->cancelled;
    P_MUTEX_UNLOCK(priv->prop_lock);
    return ret;
}

int 
QueueEvent__is_finished(QueueEvent * self){
    g_return_val_if_fail (self != NULL, TRUE);
    g_return_val_if_fail (QUEUE_IS_QUEUEEVENT (self), TRUE);
    QueueEventPrivate *priv = QueueEvent__get_instance_private (self);

    int ret;
    P_MUTEX_LOCK(priv->prop_lock);
    ret = priv->finished;
    P_MUTEX_UNLOCK(priv->prop_lock);
    return ret;
}

void 
QueueEvent__invoke(QueueEvent * self){
    g_return_if_fail (self != NULL);  //Happens if pop happens simultaniously at 0. Continue to wait on condition call
    g_return_if_fail (QUEUE_IS_QUEUEEVENT (self));
    QueueEventPrivate *priv = QueueEvent__get_instance_private (self);
    if(priv->callback){
        priv->callback(self,priv->user_data);
        P_MUTEX_LOCK(priv->prop_lock);
        if(!priv->cancelled){
            priv->finished = 1;
        }
        P_MUTEX_UNLOCK(priv->prop_lock);
        g_signal_emit (self, signals[SIGNAL_STATE_CHANGED], 0, QUEUEEVENT_DISPATCHED);
    }

    if(priv->cleanup_cb){
        priv->cleanup_cb(self, priv->cancelled, priv->user_data);
        priv->cleanup_cb = NULL;
    }

    if(priv->managed){
        g_object_unref(G_OBJECT(priv->user_data));
        priv->managed = FALSE;
    }
}

void * 
QueueEvent__get_scope(QueueEvent * self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (QUEUE_IS_QUEUEEVENT (self), NULL);
    QueueEventPrivate *priv = QueueEvent__get_instance_private (self);

    return priv->scope;
}