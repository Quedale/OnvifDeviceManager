#include "task_manager.h"

typedef struct _TaskMgr {
    GtkWidget * widget;
} TaskMgr;

void TaskMgr__create_ui(TaskMgr* self){
    self->widget = gtk_label_new("Work In Progress...");
}

TaskMgr * TaskMgr__create(){
    TaskMgr * ret = malloc(sizeof(TaskMgr));
    TaskMgr__create_ui(ret);
    return ret;
}

void TaskMgr__destroy(TaskMgr* self){
    if(self){
        free(self);
    }
}

GtkWidget * TaskMgr__get_widget(TaskMgr * self){
    return self->widget;
}