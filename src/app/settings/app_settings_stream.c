#include "app_settings_stream.h"

#define APPSETTINGS_STREAM_CAT "stream"

//Generic value callback for togglebuttons
void value_toggled (GtkToggleButton* self, AppSettingsStream * settings){
    //On value changed, send signal to check to notify parent panel
    if(settings->state_changed_callback){
        settings->state_changed_callback(settings->state_changed_user_data);
    }
}

int AppSettingsStream__get_state (AppSettingsStream * settings){
    int scale_val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(settings->overscale_chk));

    //TODO check for more settings

    //More settings widgets here
    return scale_val != settings->allow_overscale;
}

void AppSettingsStream__set_state(AppSettingsStream * self,int state){
    if(GTK_IS_WIDGET(self->overscale_chk))
        gtk_widget_set_sensitive(self->overscale_chk,state);
}

GtkWidget * AppSettingsStream__create_ui(AppSettingsStream * self){
    GtkWidget * widget = gtk_grid_new(); //Widget filling up streaming page

    //Add stream page properties
    g_object_set (widget, "margin", 20, NULL);
    gtk_widget_set_hexpand (widget, TRUE);
    self->overscale_chk = gtk_check_button_new_with_label("Allow overscaling");
    gtk_grid_attach (GTK_GRID (widget), self->overscale_chk, 0, 1, 1, 1);

    g_signal_connect (G_OBJECT (self->overscale_chk), "toggled", G_CALLBACK (value_toggled), self);

    return widget;
}


void AppSettingsStream__set_overscale_callback(AppSettingsStream * self, void (*overscale_callback)(AppSettingsStream *, int, void * ), void * overscale_userdata){
    self->overscale_callback = overscale_callback;
    self->overscale_userdata = overscale_userdata;
}

int AppSettingsStream__get_allow_overscale(AppSettingsStream * self){
    return self->allow_overscale;
}

char * AppSettingsStream__save(AppSettingsStream * self){
    if(AppSettingsStream__get_state(self) && self->overscale_callback){
        self->allow_overscale = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(self->overscale_chk));
        self->overscale_callback(self, self->allow_overscale, self->overscale_userdata);
    }
    if(self->allow_overscale){
        return "[stream]\nallow_overscaling=true";
    } else {
        return "[stream]\nallow_overscaling=false";
    }
}

void AppSettingsStream__init(AppSettingsStream * self, void (*state_changed_callback)(void * ),void * state_changed_user_data){
    self->allow_overscale = 1;
    self->overscale_callback = NULL;
    self->overscale_userdata = NULL;
    self->state_changed_callback = state_changed_callback;
    self->state_changed_user_data = state_changed_user_data;
    self->widget = AppSettingsStream__create_ui(self);
}


AppSettingsStream * AppSettingsStream__create(void (*state_changed_callback)(void * ),void * state_changed_user_data){
    AppSettingsStream * self = malloc(sizeof(AppSettingsStream));
    AppSettingsStream__init(self,state_changed_callback, state_changed_user_data);
    return self;
}

void AppSettingsStream__destroy(AppSettingsStream * self){
    free(self);
}

GtkWidget * AppSettingsStream__get_widget(AppSettingsStream * self){
    return self->widget;
}

void AppSettingsStream__reset(AppSettingsStream * self){
    if(self->allow_overscale){
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->overscale_chk),TRUE);
    } else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->overscale_chk),FALSE);
    }
}

char * AppSettingsStream__get_category(AppSettingsStream * self){
    return APPSETTINGS_STREAM_CAT;
}

int AppSettingsStream__set_property(AppSettingsStream * self, char * key, char * value){
    int valid = 0;
    if(!strcmp(key,"allow_overscaling")){
        if(!strcmp(value,"false")){
            self->allow_overscale = 0;
        } else {
            self->allow_overscale = 1;
        }
        valid = 1;
    }
    return valid;
}