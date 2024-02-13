#include "../queue/event_queue.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void evt_callback(void * user_data){
    printf("evt_callback\n");
    char * data = (char *) user_data;

    printf("data : %s\n",data);

}

void eventqueue_dispatch_cb(QueueThread * thread, EventQueueType type, void * user_data){
    EventQueue * queue = QueueThread__get_queue(thread);
    int running = EventQueue__get_running_event_count(queue);
    int pending = EventQueue__get_pending_event_count(queue);
    int total = EventQueue__get_thread_count(queue);

    char str[10];
    memset(&str,'\0',sizeof(str));
    sprintf(str, "[%d/%d]", running + pending,total);
}

int main()
{
    EventQueue * queue = EventQueue__create(eventqueue_dispatch_cb,NULL);

    EventQueue__insert(queue,evt_callback,"Data 1");
    EventQueue__insert(queue,evt_callback,"Data 2");
    EventQueue__insert(queue,evt_callback,"Data 3");
    EventQueue__insert(queue,evt_callback,"Data 4");
    EventQueue__insert(queue,evt_callback,"Data 5");
    EventQueue__insert(queue,evt_callback,"Data 6");


    EventQueue__start(queue);
    EventQueue__start(queue);
    EventQueue__start(queue);
    EventQueue__start(queue);
    while(1){
        sleep(3);
        // EventQueue__insert(queue,evt);
        // snprintf(numstr, 12, "Data %d", index++);
        // printf("buff %s\n",numstr);
        EventQueue__insert(queue,evt_callback,"Data 6");
    }
}