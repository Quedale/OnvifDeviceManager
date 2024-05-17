#include "queue_event.h"
#include "cobject.h"
#include <stdlib.h>
#include "clogger.h"

struct _QueueEvent {
    CObject parent;
    int cancelled;
    int managed;
    P_MUTEX_TYPE cancel_lock;
    void * scope;
    void * user_data;
    void (*callback)();
    void (*cleanup_cb)(int cancelled, void * user_data);
};

void priv_QueueEvent__destroy(CObject * cobject){
    QueueEvent * self = (QueueEvent *)cobject;
    P_MUTEX_CLEANUP(self->cancel_lock);
}


QueueEvent * QueueEvent__create(void * scope, void (*callback)(void * user_data), void * user_data, void (*cleanup_cb)(int cancelled, void * user_data), int managed){
    QueueEvent * self = malloc(sizeof(QueueEvent));
    QueueEvent__init(self, scope, callback, user_data, cleanup_cb, managed);
    CObject__set_allocated((CObject *)self);
    return self;
}

void QueueEvent__init(QueueEvent * self, void * scope, void (*callback)(void * user_data), void * user_data, void (*cleanup_cb)(int cancelled, void * user_data), int managed){
    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_QueueEvent__destroy);
    self->user_data = user_data;
    self->callback = callback;
    self->scope = scope;
    self->cancelled = 0;
    self->cleanup_cb = cleanup_cb;
    self->managed = managed;
    P_MUTEX_SETUP(self->cancel_lock);
}

void QueueEvent__cancel(QueueEvent * self){
    P_MUTEX_LOCK(self->cancel_lock);
    self->cancelled = 1;
    P_MUTEX_UNLOCK(self->cancel_lock);
}

int QueueEvent__is_cancelled(QueueEvent * self){
    if(!self){
        return 0;
    }
    int ret;
    P_MUTEX_LOCK(self->cancel_lock);
    ret = self->cancelled;
    P_MUTEX_UNLOCK(self->cancel_lock);
    return ret;
}

void QueueEvent__invoke(QueueEvent * self){
    if(!self || !self->callback){ //Happens if pop happens simultaniously at 0. Continue to wait on condition call
        C_TRACE("Silmutanious queue pop. Skipping");
        return;
    }
    (*(self->callback))(self->user_data);
}

void QueueEvent__cleanup(QueueEvent * self){
    if(self->cleanup_cb){
        self->cleanup_cb(self->cancelled,self->user_data);
    }
    if(self->managed){
        g_object_unref(G_OBJECT(self->user_data));
    }
}

void * QueueEvent__get_scope(QueueEvent * self){
    return self->scope;
}