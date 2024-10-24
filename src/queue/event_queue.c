
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "event_queue.h"
#include "clogger.h"
#include <string.h>

#define GLIST_FOREACH(item, list) for(GList *__glist = list; __glist && (item = __glist->data, TRUE); __glist = __glist->next)

enum {
    POOL_CHANGED,
    LAST_SIGNAL
};

typedef struct {
    GList * events;
    GList * running_events;
    GList * threads;

    P_COND_TYPE sleep_cond;
    P_MUTEX_TYPE pool_lock;
    P_MUTEX_TYPE threads_lock;
} EventQueuePrivate;

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(EventQueue, EventQueue_, G_TYPE_OBJECT)

void EventQueue__wait_finish(EventQueue* self){
    int val = -1;
    while((val = EventQueue__get_thread_count(self)) != 0){
        sleep(0.05);
    }
}

void EventQueue__finalize(GObject * obj){
    EventQueue * queue = QUEUE_EVENTQUEUE(obj);
    EventQueuePrivate *priv = EventQueue__get_instance_private (queue);

    //Thread cleanup
    int tcount = EventQueue__get_thread_count(queue);
    if(tcount){
        EventQueue__stop(queue,tcount);
        EventQueue__wait_finish(queue);
    }
    g_list_free(priv->events);
    g_list_free(priv->threads);
    g_list_free(priv->running_events);

    P_COND_CLEANUP(priv->sleep_cond);
    P_MUTEX_CLEANUP(priv->pool_lock);
    P_MUTEX_CLEANUP(priv->threads_lock);

    G_OBJECT_CLASS (EventQueue__parent_class)->finalize (obj);
}

static void
EventQueue__class_init (EventQueueClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = EventQueue__finalize;

    GType params[1];
    params[0] = QUEUE_TYPE_EVENTTYPE | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[POOL_CHANGED] =
        g_signal_newv ("pool-changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);
}

void EventQueue__init(EventQueue* self) {
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);

    priv->threads = NULL;
    priv->events = NULL;
    priv->running_events = NULL;

    P_COND_SETUP(priv->sleep_cond);
    P_MUTEX_SETUP(priv->pool_lock);
    P_MUTEX_SETUP(priv->threads_lock);
}

EventQueue* EventQueue__new() {
    C_TRACE("create...");
    return g_object_new (QUEUE_TYPE_EVENTQUEUE, NULL);
}

int EventQueue__get_running_event_count(EventQueue * self){
    g_return_val_if_fail (self != NULL,0);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self),0);
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);

    int ret;
    P_MUTEX_LOCK(priv->pool_lock);
    ret = g_list_length(priv->running_events);
    P_MUTEX_UNLOCK(priv->pool_lock);
    return ret;
}

int EventQueue__get_pending_event_count(EventQueue * self){
    g_return_val_if_fail (self != NULL,0);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self),0);
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);

    int ret;
    P_MUTEX_LOCK(priv->pool_lock);
    ret = g_list_length(priv->events);
    P_MUTEX_UNLOCK(priv->pool_lock);
    return ret;
}

int EventQueue__get_thread_count(EventQueue * self){
    g_return_val_if_fail (self != NULL,0);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self),0);
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    int ret;
    P_MUTEX_LOCK(priv->threads_lock);
    ret = g_list_length(priv->threads);
    P_MUTEX_UNLOCK(priv->threads_lock);
    return ret;
}

void EventQueue__remove_thread(EventQueue* self, QueueThread * qt){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->threads_lock);
    priv->threads = g_list_remove(priv->threads,qt);
    P_MUTEX_UNLOCK(priv->threads_lock);
    g_object_unref(qt);
}

void EventQueue__stop(EventQueue* self, int nthread){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->threads_lock);
    int cancelled_count = 0;
    GList * childs = priv->threads;
    QueueThread * thread;
    for(;childs && (thread = childs->data, TRUE); childs = childs->next){
        if(cancelled_count == nthread){
            break;
        }
        if(!QueueThread__is_cancelled(thread)){
            QueueThread__cancel(thread);
            cancelled_count++;
        }
    }
    P_COND_BROADCAST(priv->sleep_cond); //Notify thread if it's sleeping
    P_MUTEX_UNLOCK(priv->threads_lock);
}

void EventQueue__insert_private(EventQueue* self, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), void * user_data, void (*cleanup_cb)(QueueEvent * qevt, int cancelled, void * user_data), int managed){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->pool_lock);

    if(!QueueEvent__get_current() || !QueueEvent__is_cancelled(QueueEvent__get_current())){
        QueueEvent * record = QueueEvent__new(scope, callback,user_data, cleanup_cb, managed);
        priv->events = g_list_append(priv->events, record);
    } else {
        C_WARN("Ignoring event dispatched from cancelled event...");
        if(cleanup_cb){
            cleanup_cb(NULL, 1,user_data);
        }
        if(managed){
            g_object_unref(G_OBJECT(user_data));
        }
    }

    P_MUTEX_UNLOCK(priv->pool_lock);
    P_COND_SIGNAL(priv->sleep_cond);
}

void EventQueue__insert_plain(EventQueue* self, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), void * user_data, void (*cleanup_cb)(QueueEvent * qevt, int cancelled, void * user_data)){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueue__insert_private(self,scope, callback, user_data,cleanup_cb, 0);
}

void EventQueue__insert(EventQueue* self, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), gpointer user_data, void (*cleanup_cb)(QueueEvent * qevt, int cancelled, void * user_data)){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));

    if(G_IS_OBJECT(user_data)){
        g_object_ref(G_OBJECT(user_data));
    } else {
        C_FIXME("Invalid GObject. Use EventQueue_insert_plain instead.");
    }

    EventQueue__insert_private(self,scope, callback, user_data,cleanup_cb, 1);
}

void EventQueue__cancel_scopes(EventQueue * self, void ** scopes, int count){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->pool_lock);
    int i, a, evtcount;
    
    //Clean up pending events
    evtcount = g_list_length(priv->events);
    for(i=0;i<evtcount;i++){
        QueueEvent * evt = g_list_nth_data(priv->events,i);
        for(a=0;a<count;a++){
            void * scope_to_cancel = scopes[a];
            if(scope_to_cancel == QueueEvent__get_scope(evt)){
                C_INFO("Removing from queue...");
                priv->events = g_list_remove(priv->events,evt);
                i--;
                evtcount--;
                g_signal_emit (self, signals[POOL_CHANGED], 0, EVENTQUEUE_CANCELLED /* details */);
                QueueEvent__cleanup(evt);
                g_object_unref(evt);
            }
        }
    }

    //Cancellation request for running event
    evtcount = g_list_length(priv->running_events);
    for(i=0;i<evtcount;i++){
        QueueEvent * evt = g_list_nth_data(priv->running_events, i);
        for(a=0;a<count;a++){
            void * scope_to_cancel = scopes[a];
            if(scope_to_cancel == QueueEvent__get_scope(evt)){
                C_INFO("Cancelling running event...");
                QueueEvent__cancel(evt);
            }
        }
    }
    P_MUTEX_UNLOCK(priv->pool_lock);
}

QueueEvent * EventQueue__pop(EventQueue* self){
    g_return_val_if_fail (self != NULL,NULL);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self),NULL);
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->pool_lock);

    QueueEvent * qe = g_list_nth_data(priv->events, 0);
    priv->events = g_list_remove(priv->events, qe);
    priv->running_events = g_list_append(priv->running_events, qe);

    P_MUTEX_UNLOCK(priv->pool_lock);
    return qe;
}

void EventQueue__notify_cb(QueueThread * thread, QueueEventType type, EventQueue * self){
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    switch(type){
        case EVENTQUEUE_DISPATCHED:
        case EVENTQUEUE_CANCELLED:
            P_MUTEX_LOCK(priv->pool_lock);
            priv->running_events = g_list_remove(priv->running_events, QueueEvent__get_current());
            P_MUTEX_UNLOCK(priv->pool_lock);
            break;
        case EVENTQUEUE_FINISHED:
            EventQueue__remove_thread(self,thread);
            break;
        default:
            break;
    }
    
    g_signal_emit (self, signals[POOL_CHANGED], 0, type /* details */);
}

void EventQueue__start(EventQueue* self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->threads_lock);
    QueueThread * qt = QueueThread__new(self);
    g_signal_connect (G_OBJECT (qt), "state-changed", G_CALLBACK (EventQueue__notify_cb), self);

    priv->threads = g_list_append(priv->threads, qt);
    P_MUTEX_UNLOCK(priv->threads_lock);
}

void EventQueue__wait_condition(EventQueue * self, P_MUTEX_TYPE lock){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_COND_WAIT(priv->sleep_cond, lock);
}