#include "onvif_network.h"
#include "gui_utils.h"

typedef struct _OnvifNetwork {
    GtkWidget * dhcp_dd;
    GtkWidget * ip_lbl;
    GtkWidget * mask_lbl;
    GtkWidget * gateway_lbl;

    void (*done_callback)(OnvifNetwork *, void * user_data);
    void * done_user_data;

    Device * device;
    EventQueue * queue;
    GtkWidget * widget;
} OnvifNetwork;

typedef struct {
    OnvifNetwork * network;
} NetworkGUIUpdate;

gboolean * onvif_network_gui_update (void * user_data){
    NetworkGUIUpdate * update = (NetworkGUIUpdate *) user_data;

    gtk_entry_set_text(GTK_ENTRY(update->network->dhcp_dd),"test");
    gtk_editable_set_editable  ((GtkEditable*)update->network->dhcp_dd, FALSE);

    gtk_entry_set_text(GTK_ENTRY(update->network->ip_lbl),"test");
    gtk_editable_set_editable  ((GtkEditable*)update->network->ip_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(update->network->mask_lbl),"test");
    gtk_editable_set_editable  ((GtkEditable*)update->network->mask_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(update->network->gateway_lbl),"test");
    gtk_editable_set_editable  ((GtkEditable*)update->network->gateway_lbl, FALSE);

    if(update->network->done_callback)
        (*(update->network->done_callback))(update->network, update->network->done_user_data);

    free(update);

    return FALSE;
}

void _update_network_page(void * user_data){
    OnvifNetwork * self = (OnvifNetwork *) user_data;
    if(!CObject__addref((CObject*)self->device)){
        return;
    }

    if(!Device__is_selected(self->device)){
        goto exit;
    }

    NetworkGUIUpdate * gui_update = malloc(sizeof(NetworkGUIUpdate));
    gui_update->network = self;
    //TODO Fetch networking details

    gdk_threads_add_idle(G_SOURCE_FUNC(onvif_network_gui_update),gui_update);

exit:
    CObject__unref((CObject*)self->device);
}

void OnvifNetwork_update_details(OnvifNetwork * self, Device * device){
    self->device = device;
    EventQueue__insert(self->queue,_update_network_page,self);
}

void OnvifNetwork_clear_details(OnvifNetwork * self){
    gtk_entry_set_text(GTK_ENTRY(self->dhcp_dd),"");
    gtk_editable_set_editable  ((GtkEditable*)self->dhcp_dd, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->ip_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->ip_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->mask_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->mask_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->gateway_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->gateway_lbl, FALSE);

}

void OnvifNetwork__create_ui (OnvifNetwork * self){
    self->widget = gtk_grid_new ();

    self->widget = gtk_scrolled_window_new (NULL, NULL);

    GtkWidget * grid = gtk_grid_new();

    int i = 0;
    self->dhcp_dd = add_label_entry(grid,i++,"DHCP : ");
    self->ip_lbl = add_label_entry(grid,i++,"IP Address : ");
    self->mask_lbl = add_label_entry(grid,i++,"Subnet Mask : ");
    self->gateway_lbl = add_label_entry(grid,i++,"Default Gateway : ");
    
    //Hostname (Manual or DHCP)
    //DNS (Manual or DHCP)
    //NTP (Manual or DHCP)

    //HTTP (Enabled or Disabled)
    //HTTPS (Enabled or Disabled)
    //RTSP (Enabled or Disabled)

    //Enable zero-config?
    //Discoverable

    gtk_container_add(GTK_CONTAINER(self->widget),grid);
}

OnvifNetwork * OnvifNetwork__create(EventQueue * queue){
    OnvifNetwork * ret = malloc(sizeof(OnvifNetwork));
    ret->queue = queue;
    ret->device = NULL;
    ret->done_callback = NULL;
    ret->done_user_data = NULL;
    OnvifNetwork__create_ui(ret);
    return ret;
}

void OnvifNetwork__set_done_callback(OnvifNetwork* self, void (*done_callback)(OnvifNetwork *, void *), void * user_data){
    self->done_callback = done_callback;
    self->done_user_data = user_data;
}

void OnvifNetwork__destroy(OnvifNetwork* self){
    if(self){
        free(self);
    }
}

GtkWidget * OnvifNetwork__get_widget (OnvifNetwork * self){
    return self->widget;
}