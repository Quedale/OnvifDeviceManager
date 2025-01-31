#include "../queue/event_queue.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "clogger.h"

void evt_callback(QueueEvent * qevt, void * user_data){
    char * data = (char *) user_data;

    C_DEBUG("Background Task data : %s\n",data);

}

void evt_cleanup_callback(QueueEvent * qevt, int cancelled, void * user_data){

}

void eventqueue_dispatch_cb(EventQueue * queue, QueueEventType type, int running, int pending, int threadcount, QueueEvent * evt, void * self){
    char str[100];
    memset(&str,'\0',sizeof(str));
    C_DEBUG("Event %s [%d/%d]",g_enum_to_nick(QUEUE_TYPE_EVENTTYPE,type), running + pending, threadcount);
}

int main()
{
    c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);
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