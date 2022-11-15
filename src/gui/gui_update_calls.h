#ifndef GUI_CALLS_H_ 
#define GUI_CALLS_H_

#include "../queue/event_queue.h"
#include "onvif_device.h"
#include <gtk/gtk.h>

void display_onvif_thumbnail(OnvifDevice * device, EventQueue * queue, GtkWidget * handle);
void display_nslookup_hostname(OnvifDevice * device, EventQueue * queue);

#endif