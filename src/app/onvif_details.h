#ifndef ONVIF_DETAILS_H_ 
#define ONVIF_DETAILS_H_

#include "device_list.h"
#include "../queue/event_queue.h"

typedef struct _OnvifDetails OnvifDetails;

OnvifDetails * OnvifDetails__create(EventQueue * queue);
void OnvifDetails__destroy(OnvifDetails* self);
void OnvifDetails_set_details_loading_handle(OnvifDetails * self, GtkWidget * widget);
void OnvifDetails_update_details(OnvifDetails * self, Device * device);
void OnvifDetails_clear_details(OnvifDetails * self);
GtkWidget * OnvifDetails__get_widget(OnvifDetails * self);

#endif