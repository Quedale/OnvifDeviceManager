
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "event_queue.h"
#include "clist_ts.h"
#include "clogger.h"
#include <string.h>

struct _EventQueue {
    CObject parent;

    int running_events;

    CListTS events;
    CListTS threads;

    P_COND_TYPE sleep_cond;
    P_MUTEX_TYPE pool_lock;
    P_MUTEX_TYPE running_events_lock;

    void (*queue_event_cb)(QueueThread * thread, EventQueueType type, void * user_data);
    void * user_data;
};

void priv_EventQueue__wait_finish(EventQueue* self);
void priv_EventQueue__destroy(CObject * self);
void priv_EventQueue__running_event_change(EventQueue * self, int add_flag);

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

        P_COND_CLEANUP(self->sleep_cond);
        P_MUTEX_CLEANUP(self->pool_lock);
        P_MUTEX_CLEANUP(self->running_events_lock);
    }
}

void priv_EventQueue__running_event_change(EventQueue * self, int add_flag){
    P_MUTEX_LOCK(self->running_events_lock);
    if(add_flag){
        self->running_events++;
    } else {
        self->running_events--;
    }
    P_MUTEX_UNLOCK(self->running_events_lock);
}

void EventQueue__init(EventQueue* self) {
    memset (self, 0, sizeof (EventQueue));

    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_EventQueue__destroy);

    CListTS__init(&self->threads);
    CListTS__init(&self->events);

    self->running_events=0;

    P_COND_SETUP(self->sleep_cond);
    P_MUTEX_SETUP(self->pool_lock);
    P_MUTEX_SETUP(self->running_events_lock);
}

EventQueue* EventQueue__create(void (*queue_event_cb)(QueueThread * thread, EventQueueType type, void * user_data), void * user_data) {
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
    P_MUTEX_LOCK(self->running_events_lock);
    ret = self->running_events;
    P_MUTEX_UNLOCK(self->running_events_lock);
    return ret;
}

int EventQueue__get_pending_event_count(EventQueue * self){
    return CListTS__get_count(&self->events);
}

int EventQueue__get_thread_count(EventQueue * self){
    return CListTS__get_count(&self->threads);
}

void EventQueue__remove_thread(EventQueue* self, QueueThread * qt){
    CListTS__remove_record(&self->threads,(CObject*)qt);
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
    CListTS__clear(&self->events);
}

void EventQueue__insert(EventQueue* queue, void (*callback)(), void * user_data){
    C_TRACE("insert");
    if(!CObject__is_valid((CObject*)queue)){
        return;//Stop accepting events
    }
    
    QueueEvent * record = QueueEvent__create(callback,user_data);
    CListTS__add(&queue->events,(CObject*)record);
    P_COND_SIGNAL(queue->sleep_cond);

    C_TRACE("insert - done");
};

QueueEvent * EventQueue__pop(EventQueue* self){
    C_TRACE("pop");
    return (QueueEvent*) CListTS__pop(&self->events);
};

void EventQueue__start(EventQueue* self){
    P_MUTEX_LOCK(self->pool_lock);
    QueueThread * qt = QueueThread__create(self);
    CListTS__add(&self->threads,(CObject*)qt);
    P_MUTEX_UNLOCK(self->pool_lock);

    if(self->queue_event_cb){
        self->queue_event_cb(qt,EVENTQUEUE_STARTED,self->user_data);
    }
};

void EventQueue__wait_condition(EventQueue * self, P_MUTEX_TYPE lock){
    P_COND_WAIT(self->sleep_cond, lock);
}

void EventQueue_notify_dispatching(EventQueue * self, QueueThread * thread){
    C_TRACE("notify dispatching event...");
    priv_EventQueue__running_event_change(self,1);
    if(self->queue_event_cb){
        self->queue_event_cb(thread,EVENTQUEUE_DISPATCHING,self->user_data);
    }
}

void EventQueue_notify_dispatched(EventQueue * self,  QueueThread * thread){
    C_TRACE("notify dispatched event...");
    priv_EventQueue__running_event_change(self,0);
    if(self->queue_event_cb){
        self->queue_event_cb(thread,EVENTQUEUE_DISPATCHED,self->user_data);
    }
}