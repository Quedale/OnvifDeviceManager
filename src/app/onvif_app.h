
#ifndef ONVIF_APP_H_ 
#define ONVIF_APP_H_

#include "../gst/player.h"
#include "dialog/msg_dialog.h"

typedef struct _OnvifApp OnvifApp;

#include "onvif_app_shutdown.h"

OnvifApp * OnvifApp__create();
void OnvifApp__destroy(OnvifApp* self);
RtspPlayer * OnvifApp__get_player(OnvifApp* self);
MsgDialog * OnvifApp__get_msg_dialog(OnvifApp * self);

#endif