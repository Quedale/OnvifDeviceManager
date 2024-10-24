#ifndef TASK_MGR_H_ 
#define TASK_MGR_H_

#include <gtk/gtk.h>
#include "../queue/event_queue.h"
G_BEGIN_DECLS

#define ONVIFMGR_TYPE_TASKMANAGER OnvifTaskManager__get_type()
G_DECLARE_FINAL_TYPE (OnvifTaskManager, OnvifTaskManager_, ONVIFMGR, TASKMANAGER, GtkBox)

struct _OnvifTaskManager
{
  GtkBox parent_instance;
};


struct _OnvifTaskManagerClass
{
  GtkBoxClass parent_class;
};

OnvifTaskManager * OnvifTaskManager__new (EventQueue * queue);


G_END_DECLS

#endif