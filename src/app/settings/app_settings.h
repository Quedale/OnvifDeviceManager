#ifndef ONVIF_APP_SETTINGS_H_ 
#define ONVIF_APP_SETTINGS_H_

#include <gtk/gtk.h>
#include "../onvif_app.h"

#include "app_settings_stream.h"
#include "app_settings_discovery.h"

typedef struct _AppSettings AppSettings;
typedef enum _AppSettingsType {
    APPSETTING_INVALID = -1,
    APPSETTING_STREAM_TYPE = 0,
    APPSETTING_DISCOVERY_TYPE = 1,
} AppSettingsType;

struct _AppSettings {
    //Generic settings widgets
    GtkWidget * notebook;
    GtkWidget * widget;
    GtkWidget * loading_handle;
    GtkWidget * notice;
    GtkWidget * apply_button;
    GtkWidget * reset_btn;

    AppSettingsStream * stream;
    AppSettingsDiscovery * discovery;

    OnvifApp * app;
};

AppSettings * AppSettings__create(OnvifApp * app);
void AppSettings__destroy(AppSettings* dialog);
void AppSettings__set_details_loading_handle(AppSettings * self, GtkWidget * widget);
GtkWidget * AppSettings__get_widget(AppSettings * self);

#endif