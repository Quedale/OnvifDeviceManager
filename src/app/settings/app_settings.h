#ifndef ONVIF_APP_SETTINGS_H_ 
#define ONVIF_APP_SETTINGS_H_

#include <gtk/gtk.h>
#include "../../queue/event_queue.h"

typedef struct _AppSettings AppSettings;

AppSettings * AppSettings__create(EventQueue * queue);
void AppSettings__destroy(AppSettings* dialog);
void AppSettings__set_details_loading_handle(AppSettings * self, GtkWidget * widget);
void AppSettings__set_overscale_callback(AppSettings * self, void (*overscale_callback)(AppSettings *, int value, void *), void * overscale_userdata);
GtkWidget * AppSettings__get_widget(AppSettings * self);
int AppSettings__get_allow_overscale(AppSettings * self);

#endif