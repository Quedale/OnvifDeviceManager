#include "app_settings_discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define APPSETTINGS_DISCOVERY_CAT "discovery"

void dispatch_state_changed(AppSettingsDiscovery * self){
    if(self->state_changed_callback){
        self->state_changed_callback(self->state_changed_user_data);
    }
}

gboolean scale_change_value (GtkRange* scale, GtkScrollType* scroll, gdouble value, AppSettingsDiscovery * self){
    double roundedValue = round(value);
    int signal = -1;
    if(scale == GTK_RANGE(self->timeout_scale)){
        signal = self->timeout_signal;
    } else if(scale == GTK_RANGE(self->repeat_scale)){
        signal = self->repeat_signal;
    }

    if(signal > -1){
        g_signal_handler_block(scale,signal);
        g_signal_emit_by_name(scale, "change-value", scroll, roundedValue,self);
        g_signal_handler_unblock(scale,signal);
    }

    dispatch_state_changed(self);
    return TRUE;
}

int AppSettingsDiscovery__get_state (AppSettingsDiscovery * self){
    int v = gtk_range_get_value (GTK_RANGE(self->timeout_scale));
    if(v != self->timeout){
        return 1;
    }

    v = gtk_range_get_value (GTK_RANGE(self->repeat_scale));
    if(v != self->repeat){
        return 1;
    }
    return 0;
}

void AppSettingsDiscovery__set_state(AppSettingsDiscovery * self,int state){
    gtk_widget_set_sensitive(self->timeout_scale,state);
    gtk_widget_set_sensitive(self->repeat_scale,state);
}

GtkWidget * AppSettingsDiscovery__create_ui(AppSettingsDiscovery * self){
    GtkWidget * label;
    GtkWidget * widget = gtk_grid_new(); //Widget filling up streaming page

    //Add stream page properties
    g_object_set (widget, "margin", 20, NULL);
    gtk_widget_set_hexpand (widget, TRUE);

    label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label),"<span size=\"large\" ><b>Discovery timeout</b></span>");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_label_set_xalign(GTK_LABEL(label),0);
    gtk_grid_attach (GTK_GRID (widget), label, 0, 0, 1, 1);

    label = gtk_label_new("Defines how many second it should wait for a response.\nInreasing this value is useful for slow networks.");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_label_set_xalign(GTK_LABEL(label),0);
    g_object_set (label, "margin", 10, NULL);
    gtk_grid_attach (GTK_GRID (widget), label, 0, 1, 1, 1);

    self->timeout_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,1,10,1);
    gtk_widget_set_hexpand (self->timeout_scale, TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(self->timeout_scale),FALSE);
    gtk_range_set_value(GTK_RANGE(self->timeout_scale),self->timeout);
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),1,GTK_POS_BOTTOM,"1");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),2,GTK_POS_BOTTOM,"2");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),3,GTK_POS_BOTTOM,"3");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),4,GTK_POS_BOTTOM,"4");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),5,GTK_POS_BOTTOM,"5");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),6,GTK_POS_BOTTOM,"6");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),7,GTK_POS_BOTTOM,"7");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),8,GTK_POS_BOTTOM,"8");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),9,GTK_POS_BOTTOM,"9");
    gtk_scale_add_mark (GTK_SCALE(self->timeout_scale),10,GTK_POS_BOTTOM,"10");

    g_object_set (self->timeout_scale, "margin-bottom", 20, NULL);
    gtk_grid_attach (GTK_GRID (widget), self->timeout_scale, 0, 2, 1, 1);

    label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label),"<span size=\"large\" ><b>Discovery repeat count</b></span>");
    gtk_label_set_xalign(GTK_LABEL(label),0);
    gtk_grid_attach (GTK_GRID (widget), label, 0, 3, 1, 1);

    label = gtk_label_new("Defines how many times probes should be sent.\nInreasing this value is useful for less reliable networks.");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_label_set_xalign(GTK_LABEL(label),0);
    g_object_set (label, "margin", 10, NULL);
    gtk_grid_attach (GTK_GRID (widget), label, 0, 4, 1, 1);

    self->repeat_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,1,5,1);
    gtk_scale_set_draw_value(GTK_SCALE(self->repeat_scale),FALSE);
    gtk_range_set_value(GTK_RANGE(self->repeat_scale),self->repeat);
    gtk_scale_add_mark (GTK_SCALE(self->repeat_scale),1,GTK_POS_BOTTOM,"1");
    gtk_scale_add_mark (GTK_SCALE(self->repeat_scale),2,GTK_POS_BOTTOM,"2");
    gtk_scale_add_mark (GTK_SCALE(self->repeat_scale),3,GTK_POS_BOTTOM,"3");
    gtk_scale_add_mark (GTK_SCALE(self->repeat_scale),4,GTK_POS_BOTTOM,"4");
    gtk_scale_add_mark (GTK_SCALE(self->repeat_scale),5,GTK_POS_BOTTOM,"5");
    gtk_widget_set_hexpand (self->repeat_scale, TRUE);
    gtk_grid_attach (GTK_GRID (widget), self->repeat_scale, 0, 5, 1, 1);

    
    self->timeout_signal = g_signal_connect (G_OBJECT (self->timeout_scale), "change-value", G_CALLBACK (scale_change_value), self);
    self->repeat_signal = g_signal_connect (G_OBJECT (self->repeat_scale), "change-value", G_CALLBACK (scale_change_value), self);

    return widget;
}

char settings_str[100];
char * AppSettingsDiscovery__save(AppSettingsDiscovery * self){
    self->repeat = gtk_range_get_value (GTK_RANGE(self->repeat_scale));
    self->timeout = gtk_range_get_value (GTK_RANGE(self->timeout_scale));
    sprintf(settings_str, "[%s]\ntimeout=%i\nrepeat=%i",
            APPSETTINGS_DISCOVERY_CAT, self->timeout, self->repeat);
    return settings_str;
}

void AppSettingsDiscovery__init(AppSettingsDiscovery * self, void (*state_changed_callback)(void * ),void * state_changed_user_data){
    self->timeout = 2;
    self->repeat = 2;
    self->timeout_scale = NULL;
    self->repeat_scale = NULL;
    self->state_changed_callback = state_changed_callback;
    self->state_changed_user_data = state_changed_user_data;
    self->widget = AppSettingsDiscovery__create_ui(self);
}

AppSettingsDiscovery * AppSettingsDiscovery__create(void (*state_changed_callback)(void * ),void * state_changed_user_data){
    AppSettingsDiscovery * self = malloc(sizeof(AppSettingsDiscovery));
    AppSettingsDiscovery__init(self,state_changed_callback, state_changed_user_data);
    return self;
}

void AppSettingsDiscovery__destroy(AppSettingsDiscovery * self){
    free(self);
}

GtkWidget * AppSettingsDiscovery__get_widget(AppSettingsDiscovery * self){
    return self->widget;
}

void AppSettingsDiscovery__reset(AppSettingsDiscovery * self){
    gtk_range_set_value(GTK_RANGE(self->timeout_scale),self->timeout);
    gtk_range_set_value(GTK_RANGE(self->repeat_scale),self->repeat);
    dispatch_state_changed(self);
}

char * AppSettingsDiscovery__get_category(AppSettingsDiscovery * self){
    return APPSETTINGS_DISCOVERY_CAT;
}

int AppSettingsDiscovery__set_property(AppSettingsDiscovery * self, char * key, char * value){
    int valid = 0;
    if(!strcmp(key,"timeout")){
        self->timeout = atoi(value);
        valid = 1;
    } else if(!strcmp(key,"repeat")){
        self->repeat = atoi(value);
        valid = 1;
    }
    
    return valid;
}

int AppSettingsDiscovery__get_timeout(AppSettingsDiscovery * self){
    return self->timeout;
}

int AppSettingsDiscovery__get_repeat(AppSettingsDiscovery * self){
    return self->repeat;
}