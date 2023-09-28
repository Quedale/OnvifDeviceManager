#include "onvif_details.h"
#include "onvif_info.h"
#include "onvif_network.h"
#include "clogger.h"

typedef struct _OnvifDetails {
    GtkWidget * details_loading_handle;
    GtkWidget *details_notebook;
    OnvifInfo * info;
    OnvifNetwork * network;

    Device * device;
    EventQueue * queue;
    int current_page;
} OnvifDetails;

void hide_loading_cb(void * user_data){
    OnvifDetails * details = (OnvifDetails *) user_data;
    gtk_spinner_stop (GTK_SPINNER (details->details_loading_handle));
}

void network_hide_loading_cb(OnvifNetwork * self, void * user_data){
    hide_loading_cb(user_data);
}

void info_hide_loading_cb(OnvifInfo * self, void * user_data){
    hide_loading_cb(user_data);
}

void update_details_priv(OnvifDetails * self){
    if(!CObject__is_valid((CObject*)self->device)){
        return;
    }

    OnvifDevice * odev = Device__get_device(self->device);
    if(OnvifDevice__get_last_error(odev) == ONVIF_NOT_AUTHORIZED){
        return;
    }
    gtk_spinner_start (GTK_SPINNER (self->details_loading_handle));

    if(self->current_page == 0){
        OnvifInfo_update_details(self->info,self->device);
    } else if(self->current_page == 1){
        OnvifNetwork_update_details(self->network,self->device);
    }
}

void OnvifDetails__update_details(OnvifDetails * self, Device * device){
    self->device = device;
    update_details_priv(self);
}


static void details_switch_page (GtkNotebook* self, GtkWidget* page, guint page_num, OnvifDetails * details) {
    OnvifDetails__clear_details(details);

    details->current_page = page_num;
    update_details_priv(details);
}

void OnvifDetails__create_ui(OnvifDetails *self){
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

    g_signal_connect (G_OBJECT (self->details_notebook), "switch-page", G_CALLBACK (details_switch_page), self);
}

OnvifDetails * OnvifDetails__create(EventQueue * queue){
    OnvifDetails *info  =  malloc(sizeof(OnvifDetails));
    info->info = OnvifInfo__create(queue);
    info->network = OnvifNetwork__create(queue);
    info->current_page = 0;
    info->queue = queue;
    info->device = NULL;
    OnvifDetails__create_ui(info);
    OnvifInfo__set_done_callback(info->info,info_hide_loading_cb,info);
    OnvifNetwork__set_done_callback(info->network,network_hide_loading_cb,info);
    return info;
}

void OnvifDetails__destroy(OnvifDetails* self){
    if(self){
        OnvifInfo__destroy(self->info);
        free(self);
    }
}

void OnvifDetails__set_details_loading_handle(OnvifDetails * self, GtkWidget * widget){
    self->details_loading_handle = widget;
}

void OnvifDetails__clear_details(OnvifDetails * self){
    C_DEBUG("clear_details");
    if(self->current_page == 0){
        OnvifInfo_clear_details(self->info);
    } else if(self->current_page == 1){
        OnvifNetwork_clear_details(self->network);
    }

    gtk_spinner_stop (GTK_SPINNER (self->details_loading_handle));
}

GtkWidget * OnvifDetails__get_widget(OnvifDetails * self){
    return self->details_notebook;
}