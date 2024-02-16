#include "queue_thread.h"
#include "queue_event.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "cobject.h"
#include "clogger.h"

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

void * priv_QueueThread_call(void * data){
    C_DEBUG("Started...");
    QueueThread* qt = (QueueThread*) data;
    while (1){
        if(QueueThread__is_cancelled(qt)){
            goto exit;
        }
        if(EventQueue__get_pending_event_count(qt->queue) <= 0){
            P_MUTEX_LOCK(qt->sleep_lock);
            if(QueueThread__is_cancelled(qt)){
                P_MUTEX_UNLOCK(qt->sleep_lock);
                goto exit;
            }
            EventQueue__wait_condition(qt->queue,qt->sleep_lock);
            P_MUTEX_UNLOCK(qt->sleep_lock);
        }
        if(QueueThread__is_cancelled(qt)){
            goto exit;
        }

        QueueEvent * event = EventQueue__pop(qt->queue);
        QUEUE_CALLBACK callback = QueueEvent__get_callback(event);
        if(!callback){ //Happens if pop happens simultaniously at 0. Continue to wait on condition call
            continue;
        }

        EventQueue_notify_dispatching(qt->queue,qt);
        (*(callback))(QueueEvent__get_userdata(event));
        EventQueue_notify_dispatched(qt->queue,qt);
        CObject__destroy((CObject*)event);
    }

exit:
    CObject__unref((CObject*)qt);
    EventQueue__remove_thread(qt->queue,qt);
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