#ifndef GUI_CALLS_H_ 
#define GUI_CALLS_H_

#include "../queue/event_queue.h"
#include "../gst2/player.h"
#include "onvif_device.h"
#include <gtk/gtk.h>
#include "credentials_input.h"

void display_onvif_device_row(OnvifDevice * device, EventQueue * queue);
void select_onvif_device_row(OnvifPlayer * player, GtkListBoxRow * row, EventQueue * queue);
void dialog_cancel_cb(CredentialsDialog * dialog);
void dialog_login_cb(LoginEvent * input);

GtkWidget * create_controls_overlay();

#endif