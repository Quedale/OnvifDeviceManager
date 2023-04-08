#ifndef ONVIF_APP_SETTINGS_H_ 
#define ONVIF_APP_SETTINGS_H_

#include <gtk/gtk.h>
#include "../../queue/event_queue.h"

typedef struct _AppSettings AppSettings;

AppSettings * AppSettings__create(EventQueue * queue);
void AppSettings__destroy(AppSettings* dialog);
void AppSettings__set_details_loading_handle(AppSettings * self, GtkWidget * widget);
void AppSettings__update_details(AppSettings * self);
void AppSettings__clear_details(AppSettings * self);
GtkWidget * AppSettings__get_widget(AppSettings * self);

#endif