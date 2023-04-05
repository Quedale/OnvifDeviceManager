#ifndef ONVIF_NETWORK_H_ 
#define ONVIF_NETWORK_H_

#include <gtk/gtk.h>
#include "../queue/event_queue.h"
#include "device_list.h"

typedef struct _OnvifNetwork OnvifNetwork;

OnvifNetwork * OnvifNetwork__create();
void OnvifNetwork__destroy(OnvifNetwork* self);
GtkWidget * OnvifNetwork__get_widget (OnvifNetwork * self);

#endif