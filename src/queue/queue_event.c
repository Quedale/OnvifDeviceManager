#include "queue_event.h"
#include "cobject.h"
#include <stdlib.h>

struct _QueueEvent {
    CObject parent;
    void * user_data;
    void (*callback)();
};

QueueEvent * QueueEvent__create(void (*callback)(), void * user_data){
    QueueEvent * self = malloc(sizeof(QueueEvent));
    QueueEvent__init(self, callback, user_data);
    CObject__set_allocated((CObject *)self);
    return self;
}

void QueueEvent__init(QueueEvent * self, void (*callback)(), void * user_data){
    CObject__init((CObject*)self);
    self->user_data = user_data;
    self->callback = callback;
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