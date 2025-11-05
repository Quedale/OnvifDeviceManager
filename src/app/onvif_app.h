#ifndef OMGR_APP_H_ 
#define OMGR_APP_H_

#include <glib-object.h>


typedef struct _OnvifApp OnvifApp;

#include "dialog/omgr_msg_dialog.h"
#include "../queue/event_queue.h"

G_BEGIN_DECLS


#define ONVIFMGR_TYPE_APP OnvifApp__get_type()
G_DECLARE_FINAL_TYPE (OnvifApp, OnvifApp_, ONVIFMGR, APP, GtkApplication)

struct _OnvifApp {
  GtkApplication parent_instance;
};


struct _OnvifAppClass {
  GtkApplicationClass parent_class;
};

OnvifApp * OnvifApp__new (void);
void OnvifApp__destroy(OnvifApp* self);
void OnvifApp__show_msg_dialog(OnvifApp * self, OnvifMgrMsgDialog * msg_dialog);
EventQueue * OnvifApp__get_EventQueue(OnvifApp * self);

G_END_DECLS

#endif