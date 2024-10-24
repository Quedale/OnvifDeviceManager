#include "../queue/event_queue.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "clogger.h"

void evt_callback(QueueEvent * qevt, void * user_data){
    printf("evt_callback\n");
    char * data = (char *) user_data;

    printf("data : %s\n",data);

}

void evt_cleanup_callback(QueueEvent * qevt, int cancelled, void * user_data){

}

void eventqueue_dispatch_cb(EventQueue * queue, QueueEventType type,  void * self){
    int running = EventQueue__get_running_event_count(queue);
    int pending = EventQueue__get_pending_event_count(queue);
    int total = EventQueue__get_thread_count(queue);

    char str[100];
    memset(&str,'\0',sizeof(str));
    C_DEBUG("Event %s [%d/%d]",g_enum_to_nick(QUEUE_TYPE_EVENTTYPE,type), running + pending, total);
}

int main()
{
    EventQueue * queue = EventQueue__new();
    g_signal_connect (G_OBJECT(queue), "pool-changed", G_CALLBACK (eventqueue_dispatch_cb), NULL);

    char * scope1 = "1";
    char * scope2 = "2";

    EventQueue__insert_plain(queue,scope1,evt_callback,"Data 1", evt_cleanup_callback);
    EventQueue__insert_plain(queue,scope1,evt_callback,"Data 2", evt_cleanup_callback);
    EventQueue__insert_plain(queue,scope1,evt_callback,"Data 3", evt_cleanup_callback);
    EventQueue__insert_plain(queue,scope2,evt_callback,"Data 4", evt_cleanup_callback);
    EventQueue__insert_plain(queue,scope2,evt_callback,"Data 5", evt_cleanup_callback);
    EventQueue__insert_plain(queue,scope2,evt_callback,"Data 6", evt_cleanup_callback);


    EventQueue__start(queue);
    EventQueue__start(queue);
    EventQueue__start(queue);
    EventQueue__start(queue);
    while(1){
        sleep(3);
        // EventQueue__insert(queue,evt);
        // snprintf(numstr, 12, "Data %d", index++);
        // printf("buff %s\n",numstr);
        EventQueue__insert_plain(queue,NULL,evt_callback,"Data 6", evt_cleanup_callback);
    }
}