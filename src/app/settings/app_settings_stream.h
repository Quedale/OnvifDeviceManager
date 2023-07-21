#ifndef ONVIF_APP_SETTINGS_STREAM_H_ 
#define ONVIF_APP_SETTINGS_STREAM_H_

#include <gtk/gtk.h>

typedef struct _AppSettingsStream AppSettingsStream;

struct _AppSettingsStream {
    GtkWidget * widget;
    GtkWidget * overscale_chk;
    int allow_overscale;
    void (*overscale_callback)(AppSettingsStream *, int, void *);
    void * overscale_userdata;

    void (*state_changed_callback)(void * );
    void * state_changed_user_data;
};


AppSettingsStream * AppSettingsStream__create(void (*state_changed_callback)(void * ),void * state_changed_user_data);
void AppSettingsStream__set_overscale_callback(AppSettingsStream * self, void (*overscale_callback)(AppSettingsStream *, int value, void *), void * overscale_userdata);
int AppSettingsStream__get_allow_overscale(AppSettingsStream * self);
int AppSettingsStream__get_state(AppSettingsStream * settings);
void AppSettingsStream__set_state(AppSettingsStream * self,int state);
char * AppSettingsStream__save(AppSettingsStream *self);
void AppSettingsStream__reset(AppSettingsStream * settings);
char * AppSettingsStream__get_category(AppSettingsStream * self);
int AppSettingsStream__set_property(AppSettingsStream * self, char * key, char * value);
GtkWidget * AppSettingsStream__get_widget(AppSettingsStream * dialog);
void AppSettingsStream__destroy(AppSettingsStream * self);

#endif