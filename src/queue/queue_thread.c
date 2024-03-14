#include "queue_thread.h"
#include "queue_event.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "cobject.h"
#include "clogger.h"
#include "threads.h"

struct _QueueThread {
    CObject parent;
    P_THREAD_TYPE pthread;
    EventQueue * queue;
    P_MUTEX_TYPE sleep_lock;
    P_MUTEX_TYPE cancel_lock;
    int cancelled;
};

void priv_QueueThread__destroy(CObject * cobject){
    QueueThread * self = (QueueThread *)cobject;
    P_MUTEX_CLEANUP(self->sleep_lock);
    P_MUTEX_CLEANUP(self->cancel_lock);
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
    C_DEBUG("Started...");
    queue_thread = (QueueThread*) data;
    while (1){
        if(QueueThread__is_cancelled(queue_thread)){
            goto exit;
        }
        if(EventQueue__get_pending_event_count(queue_thread->queue) <= 0){
            P_MUTEX_LOCK(queue_thread->sleep_lock);
            if(QueueThread__is_cancelled(queue_thread)){
                P_MUTEX_UNLOCK(queue_thread->sleep_lock);
                goto exit;
            }
            EventQueue__wait_condition(queue_thread->queue,queue_thread->sleep_lock);
            P_MUTEX_UNLOCK(queue_thread->sleep_lock);
        }
        if(QueueThread__is_cancelled(queue_thread)){
            goto exit;
        }

        event_queue = EventQueue__pop(queue_thread->queue);
        QUEUE_CALLBACK callback = QueueEvent__get_callback(event_queue);
        if(!callback){ //Happens if pop happens simultaniously at 0. Continue to wait on condition call
            continue;
        }

        EventQueue_notify(queue_thread->queue,EVENTQUEUE_DISPATCHING);
        (*(callback))(QueueEvent__get_userdata(event_queue));
        if(QueueEvent__is_cancelled(event_queue)){
            EventQueue_notify(queue_thread->queue,EVENTQUEUE_CANCELLED);
        } else {
            EventQueue_notify(queue_thread->queue,EVENTQUEUE_DISPATCHED);
        }
        CObject__destroy((CObject*)event_queue);
    }

exit:
    CObject__unref((CObject*)queue_thread);
    EventQueue__remove_thread(queue_thread->queue,queue_thread);
    C_INFO("Finished...");
    P_THREAD_EXIT;
}

void QueueThread__init(QueueThread * self, EventQueue* queue){
    memset (self, 0, sizeof (QueueThread));

    self->queue = queue;
    CObject__init((CObject *)self);
    CObject__set_destroy_callback((CObject*)self,priv_QueueThread__destroy);
    //CObject starts with 1 reference count which is associated to the caller (CListTS will destroy child uppon destruction).
    //QueueThread needs another reference to allow finishing its run.
    CObject__addref((CObject*)self);
    
    P_MUTEX_SETUP(self->sleep_lock);
    P_MUTEX_SETUP(self->cancel_lock);
    P_THREAD_CREATE(self->pthread, priv_QueueThread_call, self);
    P_THREAD_DETACH(self->pthread);
}

QueueThread * QueueThread__create(EventQueue* queue){
    QueueThread * qt = malloc(sizeof(QueueThread));
    QueueThread__init(qt,queue);
    CObject__set_allocated((CObject *) qt);
    return qt;
}

void QueueThread__cancel(QueueThread* self){
    P_MUTEX_LOCK(self->cancel_lock);
    self->cancelled = 1;
    P_MUTEX_UNLOCK(self->cancel_lock);
}

int QueueThread__is_cancelled(QueueThread* self){
    int ret;
    P_MUTEX_LOCK(self->cancel_lock);
    ret = self->cancelled;
    P_MUTEX_UNLOCK(self->cancel_lock);
    return ret;
}

EventQueue* QueueThread__get_queue(QueueThread* self){
    return self->queue;
}