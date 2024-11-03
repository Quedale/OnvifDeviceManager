#ifndef OMGR_APP_H_ 
#define OMGR_APP_H_

#include <glib-object.h>


typedef struct _OnvifApp OnvifApp;

#include "dialog/omgr_msg_dialog.h"
#include "omgr_device_row.h"
#include "../queue/event_queue.h"

G_BEGIN_DECLS


#define ONVIFMGR_TYPE_APP OnvifApp__get_type()
G_DECLARE_FINAL_TYPE (OnvifApp, OnvifApp_, ONVIFMGR, APP, GObject)

struct _OnvifApp
{
  GObject parent_instance;
};


struct _OnvifAppClass
{
  GObjectClass parent_class;
};

OnvifApp * OnvifApp__new (void);
void OnvifApp__destroy(OnvifApp* self);
void OnvifApp__show_msg_dialog(OnvifApp * self, OnvifMgrMsgDialog * msg_dialog);
EventQueue * OnvifApp__get_EventQueue(OnvifApp * self);

G_END_DECLS

#endif