#include "onvif_details.h"
#include "onvif_info.h"
#include "onvif_network.h"

typedef struct _OnvifDetails {
    GtkWidget * details_loading_handle;
    GtkWidget *details_notebook;
    OnvifInfo * info;
    OnvifNetwork * network;
} OnvifDetails;

void hide_loading_cb(OnvifInfo * self, void * user_data){
    OnvifDetails * details = (OnvifDetails *) user_data;
    gtk_spinner_stop (GTK_SPINNER (details->details_loading_handle));
}

void OnvifDetails__create_ui(OnvifDetails *self){
    printf("OnvifDetails__create_ui\n");
    GtkWidget * widget;
    GtkWidget *label;

    self->details_notebook = gtk_notebook_new ();

    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (self->details_notebook), GTK_POS_LEFT);

    widget = OnvifInfo__get_widget(self->info);

    label = gtk_label_new ("Information");
    gtk_notebook_append_page (GTK_NOTEBOOK (self->details_notebook), widget, label);

    widget = OnvifNetwork__get_widget(self->network);

    label = gtk_label_new ("Networking");
    gtk_notebook_append_page (GTK_NOTEBOOK (self->details_notebook), widget, label);
}

OnvifDetails * OnvifDetails__create(){
    OnvifDetails *info  =  malloc(sizeof(OnvifDetails));
    info->info = OnvifInfo__create();
    info->network = OnvifNetwork__create();
    OnvifDetails__create_ui(info);
    OnvifInfo__set_done_callback(info->info,hide_loading_cb,info);
    return info;
}

void OnvifDetails__destroy(OnvifDetails* self){
    if(self){
        OnvifInfo__destroy(self->info);
        free(self);
    }
}

void OnvifDetails_set_details_loading_handle(OnvifDetails * self, GtkWidget * widget){
    self->details_loading_handle = widget;
}

void OnvifDetails_update_details(OnvifDetails * self, Device * device, EventQueue * queue){
    if(!Device__is_valid(device)){
        return;
    }
    gtk_spinner_start (GTK_SPINNER (self->details_loading_handle));
    OnvifInfo_update_details(self->info,device,queue);
}

void OnvifDetails_clear_details(OnvifDetails * self){
    OnvifInfo_clear_details(self->info);
    gtk_spinner_stop (GTK_SPINNER (self->details_loading_handle));
}

GtkWidget * OnvifDetails__get_widget(OnvifDetails * self){
    return self->details_notebook;
}