
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "event_queue.h"
#include "clogger.h"
#include <string.h>

#define GLIST_FOREACH(item, list) for(GList *__glist = list; __glist && (item = __glist->data, TRUE); __glist = __glist->next)

enum {
    POOL_CHANGED,
    EVT_ADDED,
    EVT_STARTED,
    EVT_FINISHED,
    EVT_CANCELLED,
    LAST_SIGNAL
};

typedef struct {
    GList * events;
    GList * running_events;
    GList * threads;

    P_COND_TYPE sleep_cond;
    P_MUTEX_TYPE pool_lock;
    P_MUTEX_TYPE threads_lock;
    P_MUTEX_TYPE signal_lock;
} EventQueuePrivate;

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(EventQueue, EventQueue_, G_TYPE_OBJECT)

GType
QueueEventType__get_type (void){
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

static void 
EventQueue__emit_signal_prelocked(EventQueue * self, QueueEvent * evt, QueueEventType type){
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    int runcount = 0;
    int pendingcount = 0;
    int threadcount = 0;
    if(priv->running_events) runcount = g_list_length(priv->running_events);
    if(priv->events) pendingcount = g_list_length(priv->events);
    if(priv->threads) threadcount = g_list_length(priv->threads);
    P_MUTEX_UNLOCK(priv->pool_lock);

    switch(type){
        case EVENTQUEUE_DISPATCHED:
            g_signal_emit (self, signals[EVT_FINISHED], 0, evt);
            break;
        case EVENTQUEUE_CANCELLED:
            g_signal_emit (self, signals[EVT_CANCELLED], 0, evt);
            return;
        case EVENTQUEUE_DISPATCHING:
            g_signal_emit (self, signals[EVT_STARTED], 0, evt);
            break;
        case EVENTQUEUE_ADDED:
            g_signal_emit (self, signals[EVT_ADDED], 0, evt);
            break;
        case EVENTQUEUE_STARTED:
        case EVENTQUEUE_FINISHED:
        default:
            break;
    }

    g_signal_emit (self, signals[POOL_CHANGED], 0, type, runcount, pendingcount, threadcount, evt);
}

static void 
EventQueue__emit_signal(EventQueue * self, QueueEvent * evt, QueueEventType type){
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->pool_lock);
    EventQueue__emit_signal_prelocked(self, evt, type);

}

static void 
EventQueue__stop_all_threads(EventQueue* self){
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->threads_lock);
    GList * childs = priv->threads;
    QueueThread * thread;
    int tlen = g_list_length(priv->threads);
    P_THREAD_TYPE pthread_killed[tlen];
    int index = 0;
    for(;childs && (thread = childs->data, TRUE); childs = childs->next){
        QueueThread__terminate(thread);
        P_THREAD_TYPE pthread = QueueThread__get_thread(thread);
        pthread_killed[index++] = pthread;
    }
    if(priv->threads){
        g_list_free(priv->threads);
        priv->threads = NULL;
    }
    P_MUTEX_UNLOCK(priv->threads_lock);
    P_COND_BROADCAST(priv->sleep_cond); //Notify sleeping thread

    //Thread resource clean up
    for(index=0;index<tlen;index++){
        P_THREAD_JOIN(pthread_killed[index]);
    }
}

static void 
EventQueue__dispose(GObject * obj){
    EventQueue * queue = QUEUE_EVENTQUEUE(obj);
    EventQueuePrivate *priv = EventQueue__get_instance_private (queue);

    //Thread cleanup
    EventQueue__stop_all_threads(queue);

    if(priv->events){
        //TODO Cancel pending event for clean up
        g_list_free(priv->events);
        priv->events = NULL;
    }

    if(priv->running_events){
        //Nothing to cancel here, since threads are all dead
        g_list_free(priv->running_events);
        priv->running_events = NULL;
    }

    P_COND_CLEANUP(priv->sleep_cond);
    P_MUTEX_CLEANUP(priv->pool_lock);
    P_MUTEX_CLEANUP(priv->threads_lock);
    P_MUTEX_CLEANUP(priv->signal_lock);

    G_OBJECT_CLASS (EventQueue__parent_class)->dispose (obj);
}

static void
EventQueue__class_init (EventQueueClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = EventQueue__dispose;

    GType params[5];
    params[0] = QUEUE_TYPE_EVENTTYPE | G_SIGNAL_TYPE_STATIC_SCOPE;
    params[1] = G_TYPE_INT | G_SIGNAL_TYPE_STATIC_SCOPE;
    params[2] = G_TYPE_INT | G_SIGNAL_TYPE_STATIC_SCOPE;
    params[3] = G_TYPE_INT | G_SIGNAL_TYPE_STATIC_SCOPE;
    params[4] = QUEUE_TYPE_QUEUEEVENT | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[POOL_CHANGED] =
        g_signal_newv ("pool-changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        5     /* n_params */,
                        params  /* param_types */);
    
    params[0] = QUEUE_TYPE_QUEUEEVENT | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[EVT_ADDED] =
        g_signal_newv ("evt-added",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);

    signals[EVT_STARTED] =
        g_signal_newv ("evt-started",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);

    signals[EVT_FINISHED] =
        g_signal_newv ("evt-finished",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);

    signals[EVT_CANCELLED] =
        g_signal_newv ("evt-cancelled",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);
    
}

static void 
EventQueue__init(EventQueue* self) {
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);

    priv->threads = NULL;
    priv->events = NULL;
    priv->running_events = NULL;

    P_COND_SETUP(priv->sleep_cond);
    P_MUTEX_SETUP(priv->pool_lock);
    P_MUTEX_SETUP(priv->threads_lock);
    P_MUTEX_SETUP(priv->signal_lock);
}

EventQueue* 
EventQueue__new() {
    return g_object_new (QUEUE_TYPE_EVENTQUEUE, NULL);
}

int 
EventQueue__get_thread_count(EventQueue * self){
    g_return_val_if_fail (self != NULL,0);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self),0);
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    int ret;
    P_MUTEX_LOCK(priv->threads_lock);
    ret = g_list_length(priv->threads);
    P_MUTEX_UNLOCK(priv->threads_lock);
    return ret;
}

void 
EventQueue__stop(EventQueue* self, int nthread){
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
        if(!QueueThread__is_terminated(thread)){
            QueueThread__terminate(thread);
            cancelled_count++;
        }
    }
    P_COND_BROADCAST(priv->sleep_cond); //Notify thread if it's sleeping
    P_MUTEX_UNLOCK(priv->threads_lock);
}

static void 
EventQueue__evt_state_changed_cb(QueueEvent * queue, QueueEventState state, EventQueue * self){
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    QueueEventType evt_type;
    switch(state){
        case QUEUEEVENT_DISPATCHED:
            evt_type = EVENTQUEUE_DISPATCHED;
            break;
        case QUEUEEVENT_CANCELLED:
            evt_type = EVENTQUEUE_CANCELLED;
            break;
        default:
            C_ERROR("Unknown EventQueue State [%d]",state);
            return;
    }

    P_MUTEX_LOCK(priv->pool_lock);
    priv->running_events = g_list_remove(priv->running_events, QueueEvent__get_current());
    EventQueue__emit_signal_prelocked(self, QueueEvent__get_current(), evt_type);
}

static QueueEvent * 
EventQueue__insert_private(EventQueue* self, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), void * user_data, void (*cleanup_cb)(QueueEvent * qevt, int cancelled, void * user_data), int managed){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self), NULL);
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    QueueEvent * record = NULL;
    C_TRAIL("Adding new event to queue");
    if(!QueueEvent__get_current() || !QueueEvent__is_cancelled(QueueEvent__get_current())){
        P_MUTEX_LOCK(priv->pool_lock);
        record = QueueEvent__new(scope, callback,cleanup_cb, user_data, managed);
        g_signal_connect (G_OBJECT (record), "state-changed", G_CALLBACK (EventQueue__evt_state_changed_cb), self);
        priv->events = g_list_append(priv->events, record);
        P_COND_SIGNAL(priv->sleep_cond);
        EventQueue__emit_signal_prelocked(self,record,EVENTQUEUE_ADDED);
    } else {
        C_WARN("Ignoring event dispatched from cancelled event...");
        if(cleanup_cb){
            cleanup_cb(NULL, 1,user_data);
        }
        if(managed){
            g_object_unref(G_OBJECT(user_data));
        }
    }
    return record;
}

QueueEvent * 
EventQueue__insert_plain(EventQueue* self, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), void * user_data, void (*cleanup_cb)(QueueEvent * qevt, int cancelled, void * user_data)){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self), NULL);
    return EventQueue__insert_private(self,scope, callback, user_data,cleanup_cb, 0);
}

QueueEvent * 
EventQueue__insert(EventQueue* self, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), gpointer user_data, void (*cleanup_cb)(QueueEvent * qevt, int cancelled, void * user_data)){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self), NULL);

    if(G_IS_OBJECT(user_data)){
        g_object_ref(G_OBJECT(user_data));
    } else {
        C_FIXME("Invalid GObject. Use EventQueue_insert_plain instead.");
    }

    return EventQueue__insert_private(self,scope, callback, user_data,cleanup_cb, 1);
}

static void 
EventQueue_to_notify(QueueEvent * evt, EventQueue * self){
    C_INFO("Cancelling pending event...");
    EventQueue__emit_signal(self, evt, EVENTQUEUE_CANCELLED);
    EventQueue__emit_signal(self, evt, EVENTQUEUE_FINISHED);
    g_object_unref(evt);
}

static void 
EventQueue_to_cancel(QueueEvent * evt, EventQueue * self){
    C_INFO("Cancelling running event...");
    QueueEvent__cancel(evt);
}

void 
EventQueue__cancel_scopes(EventQueue * self, void ** scopes, int count){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->pool_lock);
    int i, a, evtcount;
    
    GList * to_notify = NULL;
    GList * to_cancel = NULL;

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
                to_notify = g_list_append(to_notify, evt);
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
                priv->running_events = g_list_remove(priv->running_events,evt);
                i--;
                evtcount--;
                to_cancel = g_list_append(to_cancel, evt);
            }
        }
    }
    P_MUTEX_UNLOCK(priv->pool_lock);

    g_list_foreach(to_cancel, (GFunc)EventQueue_to_cancel, self);
    g_list_foreach(to_notify, (GFunc)EventQueue_to_notify, self);
}

QueueEvent * 
EventQueue__pop(EventQueue* self){
    g_return_val_if_fail (self != NULL,NULL);
    g_return_val_if_fail (QUEUE_IS_EVENTQUEUE (self),NULL);
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_MUTEX_LOCK(priv->pool_lock);
    if(QueueThread__is_terminated(QueueThread__get_current())){ //Check for cancelled thread after lock acquired
        P_MUTEX_UNLOCK(priv->pool_lock);
        return NULL;
    }
    QueueEvent * qe = g_list_nth_data(priv->events, 0);
    if(qe) {
        priv->events = g_list_remove(priv->events, qe);
        priv->running_events = g_list_append(priv->running_events, qe);
        EventQueue__emit_signal_prelocked(self,qe,EVENTQUEUE_DISPATCHING);
    } else {
        P_MUTEX_UNLOCK(priv->pool_lock);
    }

    return qe;
}

static void 
EventQueue__thread_state_changed_cb(QueueThread * thread, QueueThreadState state, EventQueue * self){
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    switch(state){
        case QUEUETHREAD_STARTED:
            P_MUTEX_LOCK(priv->signal_lock);
            P_MUTEX_LOCK(priv->threads_lock);
            priv->threads = g_list_append(priv->threads, thread);
            P_MUTEX_UNLOCK(priv->threads_lock);
            EventQueue__emit_signal(self, QueueEvent__get_current(), EVENTQUEUE_STARTED);
            P_MUTEX_UNLOCK(priv->signal_lock);
            break;
        case QUEUETHREAD_FINISHED:
            P_MUTEX_LOCK(priv->signal_lock);
            P_MUTEX_LOCK(priv->threads_lock);
            priv->threads = g_list_remove(priv->threads,thread);
            P_MUTEX_UNLOCK(priv->threads_lock);
            g_object_unref(thread);
            EventQueue__emit_signal(self, QueueEvent__get_current(), EVENTQUEUE_FINISHED);
            P_MUTEX_UNLOCK(priv->signal_lock);
            break;
        default:
            C_ERROR("Unknown QueueThreadState [%d]",state);
            break;
    }
}

void 
EventQueue__start(EventQueue* self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    QueueThread * qt = QueueThread__new(self);
    g_signal_connect (G_OBJECT (qt), "state-changed", G_CALLBACK (EventQueue__thread_state_changed_cb), self);
    QueueThread__start(qt);
}

void 
EventQueue__wait_condition(EventQueue * self, P_MUTEX_TYPE lock){
    g_return_if_fail (self != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE (self));
    EventQueuePrivate *priv = EventQueue__get_instance_private (self);
    P_COND_WAIT(priv->sleep_cond, lock);
}