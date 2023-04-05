#include "onvif_network.h"
#include "gui_utils.h"

typedef struct _OnvifNetwork {
    GtkWidget * dhcp_dd;
    GtkWidget * ip_lbl;
    GtkWidget * mask_lbl;
    GtkWidget * gateway_lbl;

    GtkWidget * widget;
} OnvifNetwork;

void OnvifNetwork__create_ui (OnvifNetwork * self){
    printf("OnvifNetwork__create_ui\n");
    self->widget = gtk_grid_new ();

    int i = 0;
    self->dhcp_dd = add_label_entry(self->widget,i++,"DHCP : ");
    self->ip_lbl = add_label_entry(self->widget,i++,"IP Address : ");
    self->mask_lbl = add_label_entry(self->widget,i++,"Subnet Mask : ");
    self->gateway_lbl = add_label_entry(self->widget,i++,"Default Gateway : ");
    
    //Hostname (Manual or DHCP)
    //DNS (Manual or DHCP)
    //NTP (Manual or DHCP)

    //HTTP (Enabled or Disabled)
    //HTTPS (Enabled or Disabled)
    //RTSP (Enabled or Disabled)

    //Enable zero-config?
    //Discoverable

}

OnvifNetwork * OnvifNetwork__create(){
    OnvifNetwork * ret = malloc(sizeof(OnvifNetwork));
    OnvifNetwork__create_ui(ret);
    return ret;
}

void OnvifNetwork__destroy(OnvifNetwork* self){
    if(self){
        free(self);
    }
}

GtkWidget * OnvifNetwork__get_widget (OnvifNetwork * self){
    return self->widget;
}