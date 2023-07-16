#ifndef QUEUE_EVENT_H_ 
#define QUEUE_EVENT_H_

typedef struct _QueueEvent  QueueEvent;
typedef void (*QUEUE_CALLBACK)();

QueueEvent * QueueEvent__create(void (*callback)(), void * user_data); 
void QueueEvent__init(QueueEvent * self,void (*callback)(), void * user_data);
QUEUE_CALLBACK QueueEvent__get_callback(QueueEvent * self);
void * QueueEvent__get_userdata(QueueEvent * self);

#endif