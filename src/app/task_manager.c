#include "task_manager.h"
#include "clogger.h"

enum
{
  PROP_QUEUE = 1,
  N_PROPERTIES
};


typedef struct {
    EventQueue * queue;
} OnvifTaskManagerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(OnvifTaskManager, OnvifTaskManager_, GTK_TYPE_BOX)
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

void OnvifTaskManager__eq_dispatch_cb(EventQueue * queue, QueueEventType type, OnvifTaskManager * self){
    // OnvifTaskManagerPrivate *priv = OnvifTaskManager__get_instance_private (self);

    // int running = EventQueue__get_running_event_count(queue);
    // int pending = EventQueue__get_pending_event_count(queue);
    // int total = EventQueue__get_thread_count(queue);

    // C_TRACE("EventQueue %s : %d/%d/%d",g_enum_to_nick(QUEUE_TYPE_EVENTTYPE,type),running,pending,total);

    //TODO Update Task Manager Model
}

static void
OnvifTaskManager__init (OnvifTaskManager * self)
{
    OnvifTaskManagerPrivate *priv = OnvifTaskManager__get_instance_private (self);
    priv->queue = NULL;
}

static void
OnvifTaskManager__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    OnvifTaskManager * self = ONVIFMGR_TASKMANAGER (object);
    OnvifTaskManagerPrivate *priv = OnvifTaskManager__get_instance_private (self);
    switch (prop_id){
        case PROP_QUEUE:
            priv->queue = g_value_get_object (value);
            g_signal_connect (G_OBJECT(priv->queue), "pool-changed", G_CALLBACK (OnvifTaskManager__eq_dispatch_cb), self);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifTaskManager__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    OnvifTaskManager *self = ONVIFMGR_TASKMANAGER (object);
    OnvifTaskManagerPrivate *priv = OnvifTaskManager__get_instance_private (self);
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
OnvifTaskManager__class_init (OnvifTaskManagerClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifTaskManager__set_property;
    object_class->get_property = OnvifTaskManager__get_property;
    
    obj_properties[PROP_QUEUE] =
        g_param_spec_object ("queue",
                            "EventQueue",
                            "Pointer to EventQueue parent.",
                            QUEUE_TYPE_EVENTQUEUE  /* default value */,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

OnvifTaskManager * OnvifTaskManager__new (EventQueue * queue){
    g_return_val_if_fail (queue != NULL, NULL);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE(queue), NULL);
    return g_object_new (ONVIFMGR_TYPE_TASKMANAGER, "queue", queue, NULL);
}