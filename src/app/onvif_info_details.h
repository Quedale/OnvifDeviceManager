#include <gtk/gtk.h>
#include "../queue/event_queue.h"
#include "../device_list.h"

#ifndef ONVIF_INFO_DETAILS_H_ 
#define ONVIF_INFO_DETAILS_H_

typedef struct _OnvifInfoDetails OnvifInfoDetails;

OnvifInfoDetails * OnvifInfoDetails__create();
void OnvifInfoDetails__destroy(OnvifInfoDetails* self);
void OnvifInfoDetails_update_details(OnvifInfoDetails * self, Device * device, EventQueue * queue);
void OnvifInfoDetails_clear_details(OnvifInfoDetails * self);
void OnvifInfoDetails_set_details_loading_handle(OnvifInfoDetails * self, GtkWidget * widget);
GtkWidget * OnvifInfoDetails__create_ui(OnvifInfoDetails * self);

#endif