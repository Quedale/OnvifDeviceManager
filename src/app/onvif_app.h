
#ifndef ONVIF_APP_H_ 
#define ONVIF_APP_H_


typedef struct _OnvifApp OnvifApp;

#include "../gst/player.h"
#include "dialog/msg_dialog.h"
#include "dialog/profiles_dialog.h"
#include "onvif_app_shutdown.h"

OnvifApp * OnvifApp__create();
void OnvifApp__destroy(OnvifApp* self);
RtspPlayer * OnvifApp__get_player(OnvifApp* self);
MsgDialog * OnvifApp__get_msg_dialog(OnvifApp * self);
ProfilesDialog * OnvifApp__get_profiles_dialog(OnvifApp * self);

#endif