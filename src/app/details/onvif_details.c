#include "onvif_details.h"
#include "onvif_info.h"
#include "onvif_network.h"
#include "clogger.h"

typedef struct _OnvifDetails {
    GtkWidget * details_loading_handle;
    GtkWidget *details_notebook;
    OnvifMgrDeviceRow * selected_device;
} OnvifDetails;


void OnvifDetails__hide_loading_cb(GtkWidget * widget, OnvifMgrDeviceRow * device, OnvifDetails * details){
    //Checking that the finished event is the current selected page before hiding the loading indicator (Another detail page is loading or loaded)
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(details->details_notebook));
    int widget_page = gtk_notebook_page_num(GTK_NOTEBOOK(details->details_notebook), widget);
    /*Finished can be invoked by a dangling (device change, page changed, cancelled) event.
      Make sure the finished event is the selected device*/
    if(device == details->selected_device && widget_page == current_page && GTK_IS_SPINNER(details->details_loading_handle)){
        gtk_spinner_stop (GTK_SPINNER (details->details_loading_handle));
    }
}

void OnvifDetails__show_loading_cb(GtkWidget * widget, OnvifMgrDeviceRow * device, OnvifDetails * details){
    if(GTK_IS_SPINNER(details->details_loading_handle)){
        gtk_spinner_start (GTK_SPINNER (details->details_loading_handle));
    }
    details->selected_device = device;
}

void OnvifDetails__create_ui(OnvifDetails *self, OnvifInfoPanel * info, OnvifNetworkPanel * network){
    GtkWidget *label;

    self->details_notebook = gtk_notebook_new ();

    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (self->details_notebook), GTK_POS_LEFT);

    label = gtk_label_new ("Information");
    gtk_notebook_append_page (GTK_NOTEBOOK (self->details_notebook), GTK_WIDGET(info), label);

    label = gtk_label_new ("Networking");
    gtk_notebook_append_page (GTK_NOTEBOOK (self->details_notebook), GTK_WIDGET(network), label);

}

OnvifDetails * OnvifDetails__create(OnvifApp * app){
    OnvifDetails *details  =  malloc(sizeof(OnvifDetails));
    OnvifInfoPanel * info = OnvifInfoPanel__new(app);
    OnvifNetworkPanel * network = OnvifNetworkPanel__new(app);
    OnvifDetails__create_ui(details,info,network);

    g_signal_connect (info, "finished", G_CALLBACK (OnvifDetails__hide_loading_cb), details);
    g_signal_connect (info, "started", G_CALLBACK (OnvifDetails__show_loading_cb), details);

    g_signal_connect (network, "finished", G_CALLBACK (OnvifDetails__hide_loading_cb), details);
    g_signal_connect (network, "started", G_CALLBACK (OnvifDetails__show_loading_cb), details);

    return details;
}

void OnvifDetails__destroy(OnvifDetails* self){
    if(self){
        free(self);
    }
}

void OnvifDetails__set_details_loading_handle(OnvifDetails * self, GtkWidget * widget){
    self->details_loading_handle = widget;
}

GtkWidget * OnvifDetails__get_widget(OnvifDetails * self){
    return self->details_notebook;
}