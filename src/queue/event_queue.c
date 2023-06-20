
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "event_queue.h"

 
typedef struct _EventQueue {
    int running_events;
    int event_count;
    int thread_count;
    pthread_cond_t * sleep_cond;
    pthread_mutex_t * sleep_lock;
    pthread_cond_t * pop_cond;
    pthread_mutex_t * pop_lock;

    void (*queue_event_cb)(EventQueue * queue, EventQueueType type, void * user_data);
    void * user_data;

    QueueEvent *events;
} EventQueue;

void EventQueue__init(EventQueue* self) {
    self->events=malloc(0);
    self->event_count=0;
    self->thread_count = 0;
    self->running_events=0;

    self->sleep_lock =malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(self->sleep_lock, NULL);

    self->pop_lock =malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(self->pop_lock, NULL);

    self->sleep_cond =malloc(sizeof(pthread_cond_t));
    pthread_cond_init(self->sleep_cond, NULL);
}

EventQueue* EventQueue__create(void (*queue_event_cb)(EventQueue * queue, EventQueueType type, void * user_data), void * user_data) {
    printf("EventQueue__create...\n");
    EventQueue* result = (EventQueue*) malloc(sizeof(EventQueue));
    result->queue_event_cb = queue_event_cb;
    result->user_data = user_data;
    EventQueue__init(result);
    return result;
}

int EventQueue__get_running_event_count(EventQueue * self){
    return self->running_events;
}

int EventQueue__get_pending_event_count(EventQueue * self){
    return self->event_count;
}

int EventQueue__get_thread_count(EventQueue * self){
    return self->thread_count;
}

void EventQueue__reset(EventQueue* self) {
}

void EventQueue__destroy(EventQueue* EventQueue) {
    if (EventQueue) {
        EventQueue__reset(EventQueue);
        pthread_mutex_destroy(EventQueue->sleep_lock);
        pthread_mutex_destroy(EventQueue->pop_lock);
        pthread_cond_destroy(EventQueue->sleep_cond);
        free(EventQueue->sleep_lock);
        free(EventQueue->pop_lock);
        free(EventQueue->sleep_cond);
        free(EventQueue);
    }
}

QueueEvent * EventQueue__remove_element_and_shift(EventQueue* self, QueueEvent *array, int index, int array_length)
{
    int i;
    for(i = index; i < array_length; i++) {
        array[i] = array[i + 1];
    }
    return array;
};

void EventQueue__insert_element(EventQueue* self, QueueEvent record, int index)
{ 
    int i;
    int count = self->event_count;
    self->events = (QueueEvent *) realloc (self->events,sizeof (QueueEvent) * (count+1));
    for(i=self->event_count; i> index; i--){
        self->events[i] = self->events[i-1];
    }
    self->events[index]=record;
    self->event_count++;
};

void EventQueue__clear(EventQueue* self){
    self->event_count = 0;
    self->events = realloc(self->events,0);
}

void EventQueue__remove_element(EventQueue* self, int index){
    //Remove element and shift content
    self->events = EventQueue__remove_element_and_shift(self,self->events, index, self->event_count);  /* First shift the elements, then reallocate */
    //Resize count
    self->event_count--;
    //Assign arythmatic
    int count = self->event_count; 
    //Resize array memory
    self->events = realloc (self->events,sizeof(QueueEvent) * count);
};

void EventQueue__insert(EventQueue* queue, void (*callback)(), void * user_data){
    QueueEvent record;
    record.callback = callback;
    record.user_data = user_data;
    
    pthread_mutex_lock(queue->pop_lock);
    // printf("insert empty : %i\n",record.empty);
    EventQueue__insert_element(queue,record,queue->event_count);
    if(queue->event_count >= 1){
        //Wake up thread
        pthread_cond_signal(queue->sleep_cond);
    } 
    
    pthread_mutex_unlock(queue->pop_lock);
};

QueueEvent EventQueue__pop(EventQueue* self){
    pthread_mutex_lock(self->pop_lock);
    if(self->event_count == 0){
        QueueEvent evt;
        evt.callback = NULL;
        pthread_mutex_unlock(self->pop_lock);
        return evt;
    }
    QueueEvent ret = self->events[0];
    EventQueue__remove_element(self,0);
    pthread_mutex_unlock(self->pop_lock);
    return ret;
};

void * queue_thread_cb(void * data){
    EventQueue* queue = (EventQueue*) data;
    while (1){
        if(queue->event_count <= 0){
            pthread_mutex_lock(queue->sleep_lock);
            pthread_cond_wait(queue->sleep_cond,queue->sleep_lock);
            pthread_mutex_unlock(queue->sleep_lock);
        }

        QueueEvent event = EventQueue__pop(queue);
        if(!event.callback){ //Happens if pop happens simultaniously at 0. Continue to wait on condition call
            continue;
        }
        queue->running_events++;
        if(queue->queue_event_cb){
            queue->queue_event_cb(queue,EVENTQUEUE_DISPATCHING,queue->user_data);
        }
        (*(event.callback))(event.user_data);
        queue->running_events--;
        if(queue->queue_event_cb){
            queue->queue_event_cb(queue,EVENTQUEUE_DISPATCHED,queue->user_data);
        }
    }
    return NULL;
};

void EventQueue__start(EventQueue* self){
    printf("EventQueue__start...\n");
    
    //TODO Store tid in array
    pthread_t tid;
    pthread_create(&tid, NULL, queue_thread_cb, self);
    self->thread_count++;

    if(self->queue_event_cb){
        self->queue_event_cb(self,EVENTQUEUE_STARTED,self->user_data);
    }
};