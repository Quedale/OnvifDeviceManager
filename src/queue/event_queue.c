
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "event_queue.h"
#include "clist_ts.h"
#include "clogger.h"
#include <string.h>

static const char * EVENTQUEUE_DISPATCHING_STR = "Dispatching";
static const char * EVENTQUEUE_DISPATCHED_STR = "Dispatched";
static const char * EVENTQUEUE_CANCELLED_STR = "Cancelled";
static const char * EVENTQUEUE_STARTED_STR = "Started";

struct _EventQueue {
    CObject parent;

    CListTS events;
    CListTS running_events;
    CListTS threads;

    P_COND_TYPE sleep_cond;
    P_MUTEX_TYPE pool_lock;

    void (*queue_event_cb)(EventQueue * queue, EventQueueType type, void * user_data);
    void * user_data;
};

void priv_EventQueue__wait_finish(EventQueue* self);
void priv_EventQueue__destroy(CObject * self);

const char * EventQueueType__toString(EventQueueType type){
  switch(type){
    case EVENTQUEUE_DISPATCHING:
      return EVENTQUEUE_DISPATCHING_STR;
    case EVENTQUEUE_DISPATCHED:
      return EVENTQUEUE_DISPATCHED_STR;
    case EVENTQUEUE_CANCELLED:
      return EVENTQUEUE_CANCELLED_STR;
    case EVENTQUEUE_STARTED:
      return EVENTQUEUE_STARTED_STR;
    default:
      return NULL;
  }
}

void priv_EventQueue__wait_finish(EventQueue* self){
    int val = -1;
    while((val = EventQueue__get_thread_count(self)) != 0){
        sleep(0.05);
    }
}

void priv_EventQueue__destroy(CObject * cobject){
    EventQueue * self = (EventQueue *)cobject;
    if (self) {
        //Thread cleanup
        int tcount = EventQueue__get_thread_count(self);
        if(tcount){
            EventQueue__stop(self,tcount);
            priv_EventQueue__wait_finish(self);
        }

        CObject__destroy((CObject *)&self->events);
        CObject__destroy((CObject *)&self->threads);
        CObject__destroy((CObject *)&self->running_events);

        P_COND_CLEANUP(self->sleep_cond);
        P_MUTEX_CLEANUP(self->pool_lock);
    }
}

void EventQueue__init(EventQueue* self) {
    memset (self, 0, sizeof (EventQueue));

    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_EventQueue__destroy);

    CListTS__init(&self->threads);
    CListTS__init(&self->events);
    CListTS__init(&self->running_events);

    P_COND_SETUP(self->sleep_cond);
    P_MUTEX_SETUP(self->pool_lock);
}

EventQueue* EventQueue__create(void (*queue_event_cb)(EventQueue * queue, EventQueueType type, void * user_data), void * user_data) {
    C_TRACE("create...");
    EventQueue* result = (EventQueue*) malloc(sizeof(EventQueue));
    EventQueue__init(result);
    CObject__set_allocated((CObject*)result);
    result->queue_event_cb = queue_event_cb;
    result->user_data = user_data;
    return result;
}

int EventQueue__get_running_event_count(EventQueue * self){
    int ret = -1;
    P_MUTEX_LOCK(self->pool_lock);
    ret = CListTS__get_count(&self->running_events);
    P_MUTEX_UNLOCK(self->pool_lock);
    return ret;
}

int EventQueue__get_pending_event_count(EventQueue * self){
    return CListTS__get_count(&self->events);
}

int EventQueue__get_thread_count(EventQueue * self){
    return CListTS__get_count(&self->threads);
}

void EventQueue__remove_thread(EventQueue* self, QueueThread * qt){
    CListTS__destroy_record(&self->threads,(CObject*)qt);
}

void EventQueue__stop(EventQueue* self, int nthread){
    P_MUTEX_LOCK(self->pool_lock);
    int tcount = EventQueue__get_thread_count(self);
    int cancelled_count = 0;
    for(int i=0; i < tcount; i++){
        QueueThread * thread = (QueueThread *) CListTS__get(&self->threads,i);
        if(cancelled_count == nthread){
            break;
        }
        if(!QueueThread__is_cancelled(thread)){
            QueueThread__cancel(thread);
            cancelled_count++;
        };
    }
    P_COND_BROADCAST(self->sleep_cond);
    P_MUTEX_UNLOCK(self->pool_lock);
}

void EventQueue__clear(EventQueue* self){
    //TODO Invoke cancellation and cleanup
    CListTS__clear(&self->events);
}

void EventQueue__insert(EventQueue* queue, void * scope, void (*callback)(void * user_data), void * user_data){
    if(!CObject__is_valid((CObject*)queue)){
        return;//Stop accepting events
    }
    
    P_MUTEX_LOCK(queue->pool_lock);

    /* TODO Implement cleanup mechaism before uncommenting */
    // if(!QueueEvent__is_cancelled(QueueEvent__get_current())){
        QueueEvent * record = QueueEvent__create(scope, callback,user_data);
        CListTS__add(&queue->events,(CObject*)record);
    // } else {
    //     C_WARN("Ignoring event dispatched from cancelled event...");
    // }

    P_MUTEX_UNLOCK(queue->pool_lock);
    P_COND_SIGNAL(queue->sleep_cond);
}

void EventQueue__cancel_scopes(EventQueue * self, void ** scopes, int count){
    P_MUTEX_LOCK(self->pool_lock);
    int i, a, evtcount;
    
    //Clean up pending events
    evtcount = CListTS__get_count(&self->events);
    for(i=0;i<evtcount;i++){
        QueueEvent * evt = (QueueEvent *) CListTS__get(&self->events, i);
        for(a=0;a<count;a++){
            void * scope_to_cancel = scopes[a];
            if(scope_to_cancel == QueueEvent__get_scope(evt)){
                C_INFO("Removing from queue...");
                CListTS__remove_record(&self->events,(CObject*)evt);
                i--;
                evtcount--;
                if(self->queue_event_cb){
                    self->queue_event_cb(self,EVENTQUEUE_CANCELLED,self->user_data);
                }
                CObject__destroy((CObject*)evt);
            }
        }
    }

    //Cancellation request for running event
    evtcount = CListTS__get_count(&self->running_events);
    for(i=0;i<evtcount;i++){
        QueueEvent * evt = (QueueEvent *) CListTS__get(&self->running_events, i);
        for(a=0;a<count;a++){
            void * scope_to_cancel = scopes[a];
            if(scope_to_cancel == QueueEvent__get_scope(evt)){
                C_INFO("Cancelling running event...");
                QueueEvent__cancel(evt);
            }
        }
    }
    P_MUTEX_UNLOCK(self->pool_lock);
}

QueueEvent * EventQueue__pop(EventQueue* self){
    P_MUTEX_LOCK(self->pool_lock);
    QueueEvent * qe = (QueueEvent*) CListTS__pop(&self->events);
    CListTS__add(&self->running_events,(CObject*)qe);
    P_MUTEX_UNLOCK(self->pool_lock);
    return qe;
}

void EventQueue__start(EventQueue* self){
    P_MUTEX_LOCK(self->pool_lock);
    QueueThread * qt = QueueThread__create(self);
    CListTS__add(&self->threads,(CObject*)qt);
    P_MUTEX_UNLOCK(self->pool_lock);

    if(self->queue_event_cb){
        self->queue_event_cb(self,EVENTQUEUE_STARTED,self->user_data);
    }
}

void EventQueue__wait_condition(EventQueue * self, P_MUTEX_TYPE lock){
    P_COND_WAIT(self->sleep_cond, lock);
}

void EventQueue_notify(EventQueue * self, EventQueueType type){
    C_TRACE("event notify...");
    if(type == EVENTQUEUE_DISPATCHED || type == EVENTQUEUE_CANCELLED){
        P_MUTEX_LOCK(self->pool_lock);
        CListTS__remove_record(&self->running_events,(CObject*)QueueEvent__get_current());
        P_MUTEX_UNLOCK(self->pool_lock);
    }
    if(self->queue_event_cb){
        self->queue_event_cb(self,type,self->user_data);
    }
}
