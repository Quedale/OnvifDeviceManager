#ifndef OMGR_APP_H_ 
#define OMGR_APP_H_

#include <glib-object.h>


typedef struct _OnvifApp OnvifApp;

#include "dialog/msg_dialog.h"
#include "omgr_device_row.h"
#include "../queue/queue_event.h"

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
MsgDialog * OnvifApp__get_msg_dialog(OnvifApp * self);
void OnvifApp__dispatch(OnvifApp* app, void * scope, void (*callback)(QueueEvent * qevt, void * user_data), void * user_data,void (*cleanup)(QueueEvent * qevt, int cancelled, void * user_data));

G_END_DECLS

#endif