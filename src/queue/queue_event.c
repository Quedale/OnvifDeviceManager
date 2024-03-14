#include "queue_event.h"
#include "cobject.h"
#include <stdlib.h>

struct _QueueEvent {
    CObject parent;
    int cancelled;
    P_MUTEX_TYPE cancel_lock;
    void * scope;
    void * user_data;
    void (*callback)();
};

void priv_QueueEvent__destroy(CObject * cobject){
    QueueEvent * self = (QueueEvent *)cobject;
    P_MUTEX_CLEANUP(self->cancel_lock);
}


QueueEvent * QueueEvent__create(void * scope, void (*callback)(void * user_data), void * user_data){
    QueueEvent * self = malloc(sizeof(QueueEvent));
    QueueEvent__init(self, scope, callback, user_data);
    CObject__set_allocated((CObject *)self);
    return self;
}

void QueueEvent__init(QueueEvent * self, void * scope, void (*callback)(void * user_data), void * user_data){
    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_QueueEvent__destroy);
    self->user_data = user_data;
    self->callback = callback;
    self->scope = scope;
    self->cancelled = 0;
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


QUEUE_CALLBACK QueueEvent__get_callback(QueueEvent * self){
    if(!self){
        return NULL;
    }
    return self->callback;
}

void * QueueEvent__get_userdata(QueueEvent * self){
    if(!self){
        return NULL;
    }
    return self->user_data;
}

void * QueueEvent__get_scope(QueueEvent * self){
    return self->scope;
}