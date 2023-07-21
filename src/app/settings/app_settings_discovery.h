#ifndef ONVIF_APP_DISCOVERY_STREAM_H_ 
#define ONVIF_APP_DISCOVERY_STREAM_H_

#include <gtk/gtk.h>

typedef struct _AppSettingsDiscovery AppSettingsDiscovery;

struct _AppSettingsDiscovery {
    GtkWidget * widget;
    GtkWidget * timeout_scale;
    GtkWidget * repeat_scale;

    int timeout;
    int timeout_signal;

    int repeat;
    int repeat_signal;

    void (*state_changed_callback)(void * );
    void * state_changed_user_data;
};

AppSettingsDiscovery * AppSettingsDiscovery__create(void (*state_changed_callback)(void * ),void * state_changed_user_data);
int AppSettingsDiscovery__get_state(AppSettingsDiscovery * settings);
void AppSettingsDiscovery__set_state(AppSettingsDiscovery * self,int state);
char * AppSettingsDiscovery__save(AppSettingsDiscovery *self);
void AppSettingsDiscovery__reset(AppSettingsDiscovery * settings);
char * AppSettingsDiscovery__get_category(AppSettingsDiscovery * self);
int AppSettingsDiscovery__set_property(AppSettingsDiscovery * self, char * key, char * value);
GtkWidget * AppSettingsDiscovery__get_widget(AppSettingsDiscovery * dialog);
void AppSettingsDiscovery__destroy(AppSettingsDiscovery * self);

int AppSettingsDiscovery__get_timeout(AppSettingsDiscovery * self);
int AppSettingsDiscovery__get_repeat(AppSettingsDiscovery * self);

#endif