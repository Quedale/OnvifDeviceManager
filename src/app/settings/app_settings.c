#include "app_settings.h"

typedef struct _AppSettings {
    GtkWidget * widget;
    GtkWidget * loading_handle;
    EventQueue * queue;
} AppSettings;

void AppSettings__create_ui(AppSettings * self){
    self->widget = gtk_grid_new();

}

AppSettings * AppSettings__create(EventQueue * queue){
    AppSettings * self = malloc(sizeof(AppSettings));
    self->queue = queue;
    AppSettings__create_ui(self);
    return self;
}

void AppSettings__destroy(AppSettings* self){
    if(self){
        free(self);
    }
}

void AppSettings__set_details_loading_handle(AppSettings * self, GtkWidget * widget){
    self->loading_handle = widget;
}

void AppSettings__update_details(AppSettings * self){
    printf("AppSettings_update_details\n");
}

void AppSettings__clear_details(AppSettings * self){
    printf("AppSettings_update_details\n");
}

GtkWidget * AppSettings__get_widget(AppSettings * self){
    return self->widget;
}