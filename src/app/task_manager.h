#ifndef TASK_MGR_H_ 
#define TASK_MGR_H_

#include <gtk/gtk.h>

typedef struct _TaskMgr TaskMgr;

TaskMgr * TaskMgr__create();
void TaskMgr__destroy(TaskMgr* self);
GtkWidget * TaskMgr__get_widget(TaskMgr * self);

#endif