#include "onvif_timedate.h"
#include "../gui_utils.h"
#include "clogger.h"

typedef struct {
    GtkWidget * parent;
    GtkWidget * manual_panel;
    GtkWidget * dhcp_panel;
    GtkWidget * dhcp_chck;
} OnvifTimeDatePanelPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(OnvifTimeDatePanel, OnvifTimeDatePanel_, ONVIFMGR_TYPE_DETAILSPANEL)

static void
OnvifTimeDatePanel__create_host_panel(GtkGrid * grid, OnvifHost ** hosts, int count, char * label){
    for(int i=0;i<count;i++){
        OnvifHost * host = hosts[i];
        GtkWidget * entry = gtk_entry_new();
        gtk_editable_set_editable  (GTK_EDITABLE(entry), FALSE);
        gtk_entry_set_text(GTK_ENTRY(entry),OnvifHost__getAddress(host));
        /*TODO Some indicator to display type?
           it would only be useful to potentially debug invalid ONVIF response (e.g. IPv6 type assigned to a FQDN) 
           otherwise, it should be fairly easy for a human to tell the type by looking at the address*/
        gtk_grid_attach (grid, entry, 0, i, 1, 1);
    }

    if(count < 1){
        GtkWidget * lbl = gtk_label_new("None");
        gtk_grid_attach (grid, lbl, 0, 0, 1, 1);
    }

    gtk_widget_show_all(GTK_WIDGET(grid));
}

static void 
OnvifTimeDatePanel__updateui(OnvifDetailsPanel * self, OnvifApp * app, OnvifMgrDeviceRow * device, void * user_data){
    OnvifDeviceNTP * devntp = (OnvifDeviceNTP *) user_data;
    OnvifTimeDatePanelPrivate *priv = OnvifTimeDatePanel__get_instance_private (ONVIFMGR_TIMEDATEPANEL(self));

    if(OnvifDeviceNTP__is_fromDHCP(devntp)){
        gui_widget_set_css(priv->dhcp_chck,"* { color:green; border-color:green; }");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->dhcp_chck),TRUE);
    } else {
        gui_widget_set_css(priv->dhcp_chck,"* { color:#A52A2A; border-color:#A52A2A; }");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->dhcp_chck),FALSE);
    }

    OnvifTimeDatePanel__create_host_panel(GTK_GRID(priv->dhcp_panel),OnvifDeviceNTP__get_dhcp_hosts(devntp), OnvifDeviceNTP__get_dhcp_count(devntp), "DHCP");
    OnvifTimeDatePanel__create_host_panel(GTK_GRID(priv->manual_panel),OnvifDeviceNTP__get_manual_hosts(devntp), OnvifDeviceNTP__get_manual_count(devntp), "Manual");

    gtk_widget_set_visible(priv->parent,TRUE);

    g_object_unref(devntp);
}

static void 
OnvifTimeDatePanel__clearui(OnvifDetailsPanel * self){
    OnvifTimeDatePanelPrivate *priv = OnvifTimeDatePanel__get_instance_private (ONVIFMGR_TIMEDATEPANEL(self));
    gtk_widget_set_visible(priv->parent,FALSE);
    gtk_container_foreach(GTK_CONTAINER(priv->dhcp_panel),(GtkCallback)gui_widget_destroy, NULL);
    gtk_container_foreach(GTK_CONTAINER(priv->manual_panel),(GtkCallback)gui_widget_destroy, NULL);
}

static GtkWidget*
OnvifTimeDatePanel__create_frame(OnvifTimeDatePanelPrivate *priv, GtkBox * box, char * label, gboolean dhcp){
    GtkWidget * frame = gtk_frame_new(NULL);
    gtk_frame_set_label_align(GTK_FRAME(frame),0.08,0.2);
    gtk_widget_set_margin_start(frame,10);
    gtk_widget_set_margin_end(frame,5);
    gtk_widget_set_margin_top(frame,5);

    GtkWidget * lbl = gtk_label_new(label);
    gtk_widget_set_margin_top(lbl,2);
    gtk_widget_set_margin_bottom(lbl,2);
    gtk_widget_set_margin_end(lbl,5);
    GtkWidget* lblhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    if(dhcp){
        priv->dhcp_chck = gtk_check_button_new();
        gtk_widget_set_margin_top(priv->dhcp_chck,2);
        gtk_widget_set_margin_bottom(priv->dhcp_chck,2);
        gtk_widget_set_margin_start(priv->dhcp_chck,5);
        gtk_widget_set_sensitive(priv->dhcp_chck,FALSE);

        gtk_box_pack_start(GTK_BOX(lblhbox), priv->dhcp_chck, TRUE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(lblhbox), lbl, TRUE, FALSE, 0);
    } else {
        gtk_box_pack_start(GTK_BOX(lblhbox), lbl, TRUE, FALSE, 0);
    }

    GtkWidget * titleframe = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(titleframe),lblhbox);
    gtk_frame_set_label_widget(GTK_FRAME(frame),titleframe);

    GtkWidget * grid = gtk_grid_new();
    gtk_widget_set_margin_bottom(grid,5);
    gtk_widget_set_margin_top(grid,5);
    gtk_widget_set_margin_start(grid,5);
    gtk_widget_set_margin_end(grid,5);
    gtk_grid_set_row_spacing(GTK_GRID(grid),5);

    gtk_container_add(GTK_CONTAINER(frame),grid);
    gtk_box_pack_start(box, frame, FALSE, FALSE, 0);
    
    return grid;
}

static void 
OnvifTimeDatePanel__createui(OnvifDetailsPanel * self){
    OnvifTimeDatePanelPrivate *priv = OnvifTimeDatePanel__get_instance_private (ONVIFMGR_TIMEDATEPANEL(self));

    priv->parent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(priv->parent,FALSE);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), priv->parent, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(self),hbox);
    
    priv->dhcp_panel = OnvifTimeDatePanel__create_frame(priv,GTK_BOX(priv->parent),"DHCP",TRUE);
    priv->manual_panel = OnvifTimeDatePanel__create_frame(priv,GTK_BOX(priv->parent),"Manual",FALSE);

    gtk_widget_show_all(priv->parent);

    gtk_widget_set_no_show_all(priv->parent,TRUE);
    gtk_widget_set_visible(priv->parent,FALSE);
}

static void *
OnvifTimeDatePanl__getdata(OnvifDetailsPanel * self, OnvifApp * app, OnvifMgrDeviceRow * device, QueueEvent * qevt){
    OnvifDevice * onvif_device = OnvifMgrDeviceRow__get_device(device);
    OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_device);
    if(!OnvifMgrDeviceRow__is_selected(device) || QueueEvent__is_cancelled(qevt)){
        return NULL;
    }
    OnvifDeviceNTP * devntp = OnvifDeviceService__getNTP(devserv);
    return devntp;
}

static void
OnvifTimeDatePanel__class_init (OnvifTimeDatePanelClass * klass){
    OnvifDetailsPanelClass * detklass = ONVIFMGR_DETAILSPANEL_CLASS (klass);
    detklass->updateui = OnvifTimeDatePanel__updateui;
    detklass->clearui = OnvifTimeDatePanel__clearui;
    detklass->createui = OnvifTimeDatePanel__createui;
    detklass->getdata = OnvifTimeDatePanl__getdata;
}

static void
OnvifTimeDatePanel__init (OnvifTimeDatePanel * self){

}

OnvifTimeDatePanel * 
OnvifTimeDatePanel__new(OnvifApp * app){
    if(!ONVIFMGR_IS_APP(app)){
        return NULL;
    }
    return g_object_new (ONVIFMGR_TYPE_TIMEDATEPANEL, "app", app, NULL);
}