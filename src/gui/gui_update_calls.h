#ifndef GUI_CALLS_H_ 
#define GUI_CALLS_H_

#include "../queue/event_queue.h"
#include "onvif_device.h"
#include <gtk/gtk.h>

void display_onvif_device_row(OnvifDevice * device, EventQueue * queue, GtkWidget * thumbnail_handle);

#endif