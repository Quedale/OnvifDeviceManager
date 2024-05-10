#ifndef ONVIF_APP_SETTINGS_STREAM_H_ 
#define ONVIF_APP_SETTINGS_STREAM_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define APPSETTINGS_TYPE_STREAM AppSettingsStream__get_type()
G_DECLARE_FINAL_TYPE (AppSettingsStream, AppSettingsStream_, APPSETTINGS, STREAM, GtkGrid)

struct _AppSettingsStream
{
  GtkGrid parent_instance;
};


struct _AppSettingsStreamClass
{
  GtkGridClass parent_class;
};

AppSettingsStream * AppSettingsStream__new();
int AppSettingsStream__get_view_mode(AppSettingsStream * self);
int AppSettingsStream__get_state(AppSettingsStream * settings);
void AppSettingsStream__set_state(AppSettingsStream * self,int state);
char * AppSettingsStream__save(AppSettingsStream *self);
void AppSettingsStream__reset(AppSettingsStream * settings);
char * AppSettingsStream__get_category(AppSettingsStream * self);

#endif