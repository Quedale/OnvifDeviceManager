#ifndef GUI_CALLS_H_ 
#define GUI_CALLS_H_

#include "../queue/event_queue.h"
#include "../gst2/player.h"
#include "onvif_device.h"
#include <gtk/gtk.h>

void display_onvif_device_row(OnvifDevice * device, EventQueue * queue, GtkWidget * thumbnail_handle);
void select_onvif_device_row(OnvifPlayer * player, GtkListBoxRow * row, EventQueue * queue);

#endif