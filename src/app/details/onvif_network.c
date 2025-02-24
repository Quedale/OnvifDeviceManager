#include "onvif_network.h"
#include "../gui_utils.h"
#include "clogger.h"

#define MANUAL_LABEL "Static : "
#define DHCP_LABEL "DHCP : "
#define LOCAL_LABEL "Local : "

enum {
    STARTED,
    FINISHED,
    LAST_SIGNAL
};

enum {
  PROP_APP = 1,
  N_PROPERTIES
};

typedef struct {
    OnvifApp * app;
    OnvifMgrDeviceRow * device;

    QueueEvent * previous_event;
    gulong event_signal;
} OnvifNetworkPanelPrivate;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(OnvifNetworkPanel, OnvifNetworkPanel_, GTK_TYPE_SCROLLED_WINDOW)

typedef struct {
    OnvifNetworkPanel * network;
    OnvifDeviceInterfaces * inet;

    OnvifMgrDeviceRow * device;
    QueueEvent * qevt; //Keep reference to check iscancelled before rendering result
} NetworkGUIUpdate;

void OnvifNetworkPanel_update_details(OnvifNetworkPanel * self);
void OnvifNetworkPanel_clear_details(OnvifNetworkPanel * self);

static void 
OnvifNetworkPanel__device_changed_cb(OnvifApp * app, OnvifMgrDeviceRow * device, OnvifNetworkPanel * network){
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (network);
    priv->device = device;
    OnvifNetworkPanel_clear_details(network);
    if(gtk_widget_get_mapped(GTK_WIDGET(network)) && ONVIFMGR_IS_DEVICEROW(device) && OnvifMgrDeviceRow__is_initialized(device)){
        OnvifNetworkPanel_update_details(network);
    }
}

static void 
OnvifNetworkPanel__map_event_cb (GtkWidget* self, OnvifNetworkPanel * info){
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (info);
    if(gtk_widget_get_mapped(GTK_WIDGET(info)) && ONVIFMGR_IS_DEVICEROW(priv->device) && OnvifMgrDeviceRow__is_initialized(priv->device)){
        OnvifNetworkPanel_clear_details(info);
        OnvifNetworkPanel_update_details(info);
    }
}

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

static gboolean 
onvif_network_gui_update (void * user_data){
    NetworkGUIUpdate * update = (NetworkGUIUpdate *) user_data;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(update->device) || !OnvifMgrDeviceRow__is_selected(update->device) || QueueEvent__is_cancelled(update->qevt)){
        goto exit;
    }
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(vbox,FALSE);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(update->network),hbox);

    for(int a=0;a<OnvifDeviceInterfaces__get_count(update->inet);a++){
        OnvifDeviceInterface * inet = OnvifDeviceInterfaces__get_interface(update->inet,a);
        gtk_box_pack_start(GTK_BOX(vbox), OnvifNetworkPanelPrivate__create_interface_panel(inet), FALSE, FALSE, 0);
    }

    gtk_widget_show_all(GTK_WIDGET(update->network));

    g_signal_emit (update->network, signals[FINISHED], 0, update->device);
exit:
    g_object_unref(update->inet);
    g_object_unref(update->qevt);
    g_object_unref(update->device);
    free(update);

    return FALSE;
}

static void 
update_network_event_cancelled_cb(QueueEvent * qevt, QueueEventState state, void * user_data){
    NetworkGUIUpdate * input = (NetworkGUIUpdate *) user_data;
    //Do not dispatch on QUEUEEVENT_DISPATCHED.
    //FINISHED will be invoked at the end of the GUI update
    if(state == QUEUEEVENT_CANCELLED)
        gui_signal_emit(input->network, signals[FINISHED], input->device);
}

static void 
_update_network_page_cleanup(QueueEvent * qevt, int cancelled, void * user_data){
    NetworkGUIUpdate * input = (NetworkGUIUpdate *) user_data;
    g_object_unref(input->device);
    free(input);
}

static void 
_update_network_page(QueueEvent * qevt, void * user_data){
    NetworkGUIUpdate * input = (NetworkGUIUpdate *) user_data;
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(input->device) || !OnvifMgrDeviceRow__is_selected(input->device) || QueueEvent__is_cancelled(qevt)){
        return;
    }

    OnvifDevice * onvif_device = OnvifMgrDeviceRow__get_device(input->device);
    OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_device);
    OnvifDeviceInterfaces * inet = OnvifDeviceService__getNetworkInterfaces(devserv);
    if(!OnvifMgrDeviceRow__is_selected(input->device) || QueueEvent__is_cancelled(qevt)){
        return;
    }

    NetworkGUIUpdate * gui_update = malloc(sizeof(NetworkGUIUpdate));
    gui_update->network = input->network;
    gui_update->qevt = qevt;
    gui_update->device = input->device;
    gui_update->inet = inet;
    g_object_ref(input->device); //Adding reference for gui thread
    g_object_ref(gui_update->qevt);
    gdk_threads_add_idle(G_SOURCE_FUNC(onvif_network_gui_update),gui_update);
}

void 
OnvifNetworkPanel_update_details(OnvifNetworkPanel * self){
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (self);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(priv->device)){
        return;
    }

    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(priv->device);
    if(!OnvifDevice__is_authenticated(odev)){
        return;
    }

    NetworkGUIUpdate * input = malloc(sizeof(NetworkGUIUpdate));
    input->network = self;
    input->inet = NULL;
    input->device = priv->device;
    g_object_ref(input->device); //Referenced cleaned up in _update_network_page_cleanup

    if(priv->previous_event && !QueueEvent__is_finished(priv->previous_event)) {
        g_signal_handler_disconnect(priv->previous_event,priv->event_signal); //Removing signal to prevent loading from hiding
        QueueEvent__cancel(priv->previous_event);
        g_object_unref(priv->previous_event);
        priv->previous_event = NULL;
        priv->event_signal = 0;
    } else if(priv->previous_event){
        g_object_unref(priv->previous_event);
    }

    g_signal_emit (self, signals[STARTED], 0, input->device);
    priv->previous_event = EventQueue__insert_plain(OnvifApp__get_EventQueue(priv->app), input->device, _update_network_page,input, _update_network_page_cleanup);
    priv->event_signal = g_signal_connect (priv->previous_event, "state-changed", G_CALLBACK (update_network_event_cancelled_cb), input);
    g_object_ref(priv->previous_event);
}

void OnvifNetworkPanel_clear_details(OnvifNetworkPanel * self){
    gtk_container_foreach(GTK_CONTAINER(self),(GtkCallback)gui_widget_destroy, NULL);
}

static void
OnvifNetworkPanel__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifNetworkPanel * info = ONVIFMGR_NETWORKPANEL (object);
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (info);
    switch (prop_id){
        case PROP_APP:
            priv->app = ONVIFMGR_APP(g_value_get_object (value));
            g_signal_connect (priv->app, "device-changed", G_CALLBACK (OnvifNetworkPanel__device_changed_cb), info);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifNetworkPanel__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifNetworkPanel *info = ONVIFMGR_NETWORKPANEL (object);
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (info);
    switch (prop_id){
        case PROP_APP:
            g_value_set_object (value, priv->app);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifNetworkPanel__class_init (OnvifNetworkPanelClass * klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = OnvifNetworkPanel__set_property;
    object_class->get_property = OnvifNetworkPanel__get_property;

    obj_properties[PROP_APP] =
        g_param_spec_object ("app",
                            "OnvifApp",
                            "Pointer to OnvifApp parent.",
                            ONVIFMGR_TYPE_APP  /* default value */,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    GType params[1];
    params[0] = ONVIFMGR_TYPE_DEVICEROW | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[STARTED] =
        g_signal_newv ("started",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);

    signals[FINISHED] =
        g_signal_newv ("finished",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);
                        
    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

static void
OnvifNetworkPanel__init (OnvifNetworkPanel * self){
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (self);
    priv->app = NULL;
    priv->device = NULL;

    g_signal_connect (self, "map", G_CALLBACK (OnvifNetworkPanel__map_event_cb), self);
}

OnvifNetworkPanel * 
OnvifNetworkPanel__new(OnvifApp * app){
    if(!ONVIFMGR_IS_APP(app)){
        return NULL;
    }
    return g_object_new (ONVIFMGR_TYPE_NETWORKPANEL, "app", app, NULL);
}