#include "onvif_network.h"
#include "../gui_utils.h"
#include "clogger.h"

#define MANUAL_LABEL "Static : "
#define DHCP_LABEL "DHCP : "
#define LOCAL_LABEL "Local : "


G_DEFINE_TYPE(OnvifNetworkPanel, OnvifNetworkPanel_, ONVIFMGR_TYPE_DETAILSPANEL)

static void
OnvifNetworkPanelPrivate__add_ipaddr(OnvifPrefixedIPAddress ** addrs, int count, char * label, GtkWidget * grid, int * row_index){
    GtkWidget * widget;
    OnvifPrefixedIPAddress * addr;
    for(int i=0;i<count;i++){
        addr = addrs[i];
        widget = gtk_label_new(label);
        gtk_label_set_xalign (GTK_LABEL(widget), 1.0);
        gtk_grid_attach (GTK_GRID (grid), widget, 0, *row_index, 1, 1);

        widget = gtk_entry_new();
        gtk_entry_set_max_width_chars(GTK_ENTRY(widget),23);
        gtk_editable_set_editable(GTK_EDITABLE(widget),FALSE);
        char strIp[strlen(OnvifPrefixedIPAddress__get_address(addr)) + 4];
        sprintf(strIp, "%s/%d", OnvifPrefixedIPAddress__get_address(addr),OnvifPrefixedIPAddress__get_prefix(addr));
        gtk_entry_set_text(GTK_ENTRY(widget),strIp);
        gtk_grid_attach (GTK_GRID (grid), widget, 1, *row_index, 1, 1);
        *row_index =  *row_index + 1;
    }
}

static void
OnvifNetworkPanelPrivate__create_ips_panel(OnvifDeviceInterface * inet, GtkWidget * grid, int * row_index){
    OnvifIPv4Configuration * v4 = OnvifDeviceInterface__get_ipv4(inet);
    if(v4){
        OnvifPrefixedIPAddress * addr = OnvifIPv4Configuration__get_fromdhcp(v4);
        if(addr)
            OnvifNetworkPanelPrivate__add_ipaddr(&addr,1,DHCP_LABEL,grid,row_index);

        addr = OnvifIPv4Configuration__get_local(v4);
        if(addr)
            OnvifNetworkPanelPrivate__add_ipaddr(&addr,1,LOCAL_LABEL,grid,row_index);

        OnvifNetworkPanelPrivate__add_ipaddr(OnvifIPv4Configuration__get_manuals(v4),
                                            OnvifIPv4Configuration__get_manual_count(v4),
                                            MANUAL_LABEL,
                                            grid,
                                            row_index);
    }

    OnvifIPv6Configuration * v6 = OnvifDeviceInterface__get_ipv6(inet);
    if(v6){
        OnvifNetworkPanelPrivate__add_ipaddr(OnvifIPv6Configuration__get_fromdhcp(v6),
                                            OnvifIPv6Configuration__get_fromdhcp_count(v6),
                                            DHCP_LABEL,
                                            grid,
                                            row_index);
        OnvifNetworkPanelPrivate__add_ipaddr(OnvifIPv6Configuration__get_local(v6),
                                            OnvifIPv6Configuration__get_local_count(v6),
                                            LOCAL_LABEL,
                                            grid,
                                            row_index);
        OnvifNetworkPanelPrivate__add_ipaddr(OnvifIPv6Configuration__get_manuals(v6),
                                            OnvifIPv6Configuration__get_manual_count(v6),
                                            MANUAL_LABEL,
                                            grid,
                                            row_index);
        OnvifNetworkPanelPrivate__add_ipaddr(OnvifIPv6Configuration__get_fromra(v6),
                                            OnvifIPv6Configuration__get_fromra_count(v6),
                                            "RA : ",
                                            grid,
                                            row_index);
    }
}

static GtkWidget * 
OnvifNetworkPanelPrivate__create_interface_panel(OnvifDeviceInterface * inet){
    GtkWidget * frame = gtk_frame_new(NULL);
    gtk_frame_set_label_align(GTK_FRAME(frame),0.08,0.2);
    gtk_widget_set_margin_start(frame,10);
    gtk_widget_set_margin_end(frame,5);
    gtk_widget_set_margin_top(frame,5);

    GtkWidget * grid = gtk_grid_new();
    gtk_widget_set_margin_bottom(grid,5);
    gtk_widget_set_margin_top(grid,5);
    gtk_widget_set_margin_start(grid,5);
    gtk_widget_set_margin_end(grid,5);
    gtk_grid_set_row_spacing(GTK_GRID(grid),5);

    gtk_container_add(GTK_CONTAINER(frame),grid);

    int row_index = 0;

    char mtu[10];
    sprintf(mtu, "%d", OnvifDeviceInterface__get_mtu(inet));

    GtkWidget * widget;

    widget = gtk_check_button_new();
    gtk_widget_set_margin_top(widget,2);
    gtk_widget_set_margin_bottom(widget,2);
    gtk_widget_set_margin_start(widget,5);
    gtk_widget_set_sensitive(widget,FALSE);    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),OnvifDeviceInterface__get_enabled(inet));
    GtkWidget * lbl = gtk_label_new(OnvifDeviceInterface__get_name(inet));
    gtk_widget_set_margin_top(lbl,2);
    gtk_widget_set_margin_bottom(lbl,2);
    gtk_widget_set_margin_end(lbl,5);
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), lbl, TRUE, FALSE, 0);
    GtkWidget * titleframe = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(titleframe),hbox);
    gtk_frame_set_label_widget(GTK_FRAME(frame),titleframe);

    if(OnvifDeviceInterface__get_enabled(inet)){
        gui_widget_set_css(widget,"* { color:green; border-color:green; }");
    } else {
        gui_widget_set_css(widget,"* { color:#A52A2A; border-color:#A52A2A; }");
    }

    widget = gtk_label_new("MAC : ");
    gtk_widget_set_hexpand(widget,TRUE);
    gtk_label_set_xalign (GTK_LABEL(widget), 1.0);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, row_index, 1, 1);

    widget = gtk_entry_new();
    gtk_entry_set_max_width_chars(GTK_ENTRY(widget),23);
    gtk_editable_set_editable(GTK_EDITABLE(widget),FALSE);
    if(OnvifDeviceInterface__get_mac(inet))
        gtk_entry_set_text(GTK_ENTRY(widget), OnvifDeviceInterface__get_mac(inet));
    else
         gtk_entry_set_text(GTK_ENTRY(widget), "MAC Address not defined.");
    gtk_grid_attach (GTK_GRID (grid), widget, 1, row_index, 1, 1);
    row_index++;

    widget = gtk_label_new("MTU : ");
    gtk_widget_set_hexpand(widget,TRUE);
    gtk_label_set_xalign (GTK_LABEL(widget), 1.0);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, row_index, 1, 1);

    widget = gtk_entry_new();
    gtk_entry_set_max_width_chars(GTK_ENTRY(widget),23);
    gtk_entry_set_text(GTK_ENTRY(widget), mtu);
    gtk_editable_set_editable(GTK_EDITABLE(widget),FALSE);


    gtk_grid_attach (GTK_GRID (grid), widget, 1, row_index, 1, 1);
    row_index++;
    
    OnvifNetworkPanelPrivate__create_ips_panel(inet, grid, &row_index);

    return frame;
}

static void 
OnvifNetworkPanel__updateui (OnvifDetailsPanel * self, OnvifApp * app, OnvifMgrDeviceRow * device, void * user_data){

    OnvifDeviceInterfaces * inets = (OnvifDeviceInterfaces *) user_data;

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(vbox,FALSE);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(self),hbox);

    for(int a=0;a<OnvifDeviceInterfaces__get_count(inets);a++){
        OnvifDeviceInterface * inet = OnvifDeviceInterfaces__get_interface(inets,a);
        gtk_box_pack_start(GTK_BOX(vbox), OnvifNetworkPanelPrivate__create_interface_panel(inet), FALSE, FALSE, 0);
    }

    gtk_widget_show_all(GTK_WIDGET(self));

    g_object_unref(inets);
}

static void * 
OnvifNetworkPanel__getdata(OnvifDetailsPanel * self, OnvifApp * app, OnvifMgrDeviceRow * device, QueueEvent * qevt){
    OnvifDevice * onvif_device = OnvifMgrDeviceRow__get_device(device);
    OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_device);
    if(!OnvifMgrDeviceRow__is_selected(device) || QueueEvent__is_cancelled(qevt)){
        return NULL;
    }
    OnvifDeviceInterfaces * inet = OnvifDeviceService__getNetworkInterfaces(devserv);
    return inet;
}

static void 
OnvifNetworkPanel_clearui(OnvifDetailsPanel * self){
    gtk_container_foreach(GTK_CONTAINER(self),(GtkCallback)gui_widget_destroy, NULL);
}

static void
OnvifNetworkPanel__class_init (OnvifNetworkPanelClass * klass){
    OnvifDetailsPanelClass * detailsp_klass = ONVIFMGR_DETAILSPANEL_CLASS(klass);
    detailsp_klass->getdata = OnvifNetworkPanel__getdata;
    detailsp_klass->clearui = OnvifNetworkPanel_clearui;
    detailsp_klass->createui = NULL; //Dynamic UI
    detailsp_klass->updateui = OnvifNetworkPanel__updateui; 

}

static void
OnvifNetworkPanel__init (OnvifNetworkPanel * self){

}

OnvifNetworkPanel * 
OnvifNetworkPanel__new(OnvifApp * app){
    if(!ONVIFMGR_IS_APP(app)){
        return NULL;
    }
    return g_object_new (ONVIFMGR_TYPE_NETWORKPANEL, "app", app, NULL);
}