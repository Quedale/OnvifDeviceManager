#ifndef DEVICE_LIST_H_ 
#define DEVICE_LIST_H_

#include "onvif_device.h"
#include <gtk/gtk.h>
#include "../queue/event_queue.h"
#include "cobject.h"
#include "clist.h"

typedef struct _Device Device;

Device * Device__create(OnvifDevice * onvif_device);
void Device__set_profile_callback(Device * device, void (*profile_callback)(Device *, void *), void * profile_userdata);
void Device__lookup_hostname(Device* device, EventQueue * queue); 
void Device__load_thumbnail(Device* device, EventQueue * queue);
void Device__load_profiles(Device* device, EventQueue * queue);
GtkWidget * Device__create_row (Device * device, char * uri, char* name, char * hardware, char * location);
OnvifDevice * Device__get_device(Device * self);
int Device__is_selected(Device * self);
void Device__set_selected(Device * self, int selected);
int Device__get_selected_profile(Device * self);
void Device__set_thumbnail(Device * self, GtkWidget * image);

#endif