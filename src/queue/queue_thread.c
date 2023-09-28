#include "queue_thread.h"
#include "queue_event.h"
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "cobject.h"
#include "clogger.h"

struct _QueueThread {
    CObject parent;
    pthread_t pthread;
    EventQueue * queue;
    pthread_mutex_t sleep_lock;
    pthread_mutex_t cancel_lock;
    int cancelled;
};

void * priv_QueueThread_call(void * data){
    C_DEBUG("Started...");
    QueueThread* qt = (QueueThread*) data;
    while (1){
        if(QueueThread__is_cancelled(qt)){
            goto exit;
        }
        if(EventQueue__get_pending_event_count(qt->queue) <= 0){
            pthread_mutex_lock(&qt->sleep_lock);
            if(QueueThread__is_cancelled(qt)){
                pthread_mutex_unlock(&qt->sleep_lock);
                goto exit;
            }
            EventQueue__wait_condition(qt->queue,&qt->sleep_lock);
            pthread_mutex_unlock(&qt->sleep_lock);
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
    C_INFO("Finished...");
    EventQueue__remove_thread(qt->queue,qt);
    pthread_exit(0);
};

void QueueThread__init(QueueThread * self, EventQueue* queue){
    memset (self, 0, sizeof (QueueThread));

    self->queue = queue;

    pthread_mutex_init(&self->sleep_lock, NULL);

    pthread_mutex_init(&self->cancel_lock, NULL);

    pthread_create(&self->pthread, NULL, priv_QueueThread_call, self);

    CObject__init((CObject *)self);
}

QueueThread * QueueThread__create(EventQueue* queue){
    QueueThread * qt = malloc(sizeof(QueueThread));
    QueueThread__init(qt,queue);

    return qt;
}

void QueueThread__cancel(QueueThread* self){
    pthread_mutex_lock(&self->cancel_lock);
    self->cancelled = 1;
    pthread_mutex_unlock(&self->cancel_lock);
}

int QueueThread__is_cancelled(QueueThread* self){
    int ret;
    pthread_mutex_lock(&self->cancel_lock);
    ret = self->cancelled;
    pthread_mutex_unlock(&self->cancel_lock);
    return ret;
}

EventQueue* QueueThread__get_queue(QueueThread* self){
    return self->queue;
}