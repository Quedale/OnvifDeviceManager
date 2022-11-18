#include "../queue/event_queue.h"
#include <stdio.h>
#include <unistd.h>

void evt_callback(void * user_data){
    printf("evt_callback\n");
    char * data = (char *) user_data;

    printf("data : %s\n",data);

}

int main()
{
    EventQueue * queue = EventQueue__create();

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
};