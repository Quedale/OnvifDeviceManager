#include "onvif_network.h"
#include "gui_utils.h"
#include "clogger.h"

enum {
    STARTED,
    FINISHED,
    LAST_SIGNAL
};

enum
{
  PROP_APP = 1,
  N_PROPERTIES
};

typedef struct {
    OnvifApp * app;
    OnvifMgrDeviceRow * device;

    GtkWidget * dhcp_dd;
    GtkWidget * ip_lbl;
    GtkWidget * mask_lbl;
    GtkWidget * gateway_lbl;
} OnvifNetworkPanelPrivate;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(OnvifNetworkPanel, OnvifNetworkPanel_, GTK_TYPE_SCROLLED_WINDOW)

typedef struct {
    OnvifNetworkPanel * network;
    OnvifMgrDeviceRow * device;
    //Retrieved data field will be here
} NetworkGUIUpdate;

typedef struct {
    OnvifNetworkPanel * network;
    OnvifMgrDeviceRow * device;
} NetworkDataUpdate;

void OnvifNetworkPanel_update_details(OnvifNetworkPanel * self, OnvifMgrDeviceRow * device);
void OnvifNetworkPanel_clear_details(OnvifNetworkPanel * self);

void OnvifNetworkPanel__create_ui (OnvifNetworkPanel * self){

    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (self);

    GtkWidget * grid = gtk_grid_new();

    int i = 0;
    priv->dhcp_dd = add_label_entry(grid,i++,"DHCP : ");
    priv->ip_lbl = add_label_entry(grid,i++,"IP Address : ");
    priv->mask_lbl = add_label_entry(grid,i++,"Subnet Mask : ");
    priv->gateway_lbl = add_label_entry(grid,i++,"Default Gateway : ");
    
    //Hostname (Manual or DHCP)
    //DNS (Manual or DHCP)
    //NTP (Manual or DHCP)

    //HTTP (Enabled or Disabled)
    //HTTPS (Enabled or Disabled)
    //RTSP (Enabled or Disabled)

    //Enable zero-config?
    //Discoverable

    gtk_container_add(GTK_CONTAINER(self),grid);
}

void OnvifNetworkPanel__device_changed_cb(OnvifApp * app, OnvifMgrDeviceRow * device, OnvifNetworkPanel * network){
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (network);
    priv->device = device;
    OnvifNetworkPanel_clear_details(network);
    if(gtk_widget_get_mapped(GTK_WIDGET(network)) && ONVIFMGR_IS_DEVICEROW(device) && OnvifMgrDeviceRow__is_initialized(device)){
        OnvifNetworkPanel_update_details(network,priv->device);
    }
}

void OnvifNetworkPanel__map_event_cb (GtkWidget* self, OnvifNetworkPanel * info){
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (info);
    if(gtk_widget_get_mapped(GTK_WIDGET(info)) && ONVIFMGR_IS_DEVICEROW(priv->device) && OnvifMgrDeviceRow__is_initialized(priv->device)){
        OnvifNetworkPanel_clear_details(info);
        OnvifNetworkPanel_update_details(info,priv->device);
    }
}

gboolean * onvif_network_gui_update (void * user_data){
    NetworkGUIUpdate * update = (NetworkGUIUpdate *) user_data;
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(update->device)){
        C_TRAIL("onvif_info_gui_update - invalid device.");
        goto exit;
    }

    if(!OnvifMgrDeviceRow__is_selected(update->device)){
        goto exit;
    }
    
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (update->network);

    gtk_entry_set_text(GTK_ENTRY(priv->dhcp_dd),"test");
    gtk_editable_set_editable  ((GtkEditable*)priv->dhcp_dd, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->ip_lbl),"test");
    gtk_editable_set_editable  ((GtkEditable*)priv->ip_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->mask_lbl),"test");
    gtk_editable_set_editable  ((GtkEditable*)priv->mask_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->gateway_lbl),"test");
    gtk_editable_set_editable  ((GtkEditable*)priv->gateway_lbl, FALSE);

    g_signal_emit (update->network, signals[FINISHED], 0, update->device);
exit:
    g_object_unref(update->device);
    free(update);

    return FALSE;
}

void _update_network_page_cleanup(int cancelled, void * user_data){
    NetworkDataUpdate * input = (NetworkDataUpdate *) user_data;
    if(cancelled){
        //If cancelled, dispatch finish signal
        gui_signal_emit(input->network, signals[FINISHED],input->device);
    }
    g_object_unref(input->device);
    free(input);
}

void _update_network_page(void * user_data){
    NetworkDataUpdate * input = (NetworkDataUpdate *) user_data;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(input->device)){
        C_TRAIL("_update_network_page - invalid device.");
        return;
    }

    if(!OnvifMgrDeviceRow__is_selected(input->device)){
        return;
    }

    NetworkGUIUpdate * gui_update = malloc(sizeof(NetworkGUIUpdate));
    gui_update->network = input->network;
    gui_update->device = input->device;
    //TODO Fetch networking details

    g_object_ref(input->device);
    gdk_threads_add_idle(G_SOURCE_FUNC(onvif_network_gui_update),gui_update);
}

void OnvifNetworkPanel_update_details(OnvifNetworkPanel * self, OnvifMgrDeviceRow * device){
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device)){
        C_TRAIL("update_details_priv - invalid device.");
        return;
    }

    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(device);
    if(!OnvifDevice__is_authenticated(odev)){
        return;
    }
    
    g_signal_emit (self, signals[STARTED], 0, device);

    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (self);

    NetworkDataUpdate * input = malloc(sizeof(NetworkDataUpdate));
    input->device = device;
    input->network = self;
    g_object_ref(device);
    OnvifApp__dispatch(priv->app, device, _update_network_page,input,_update_network_page_cleanup);
}

void OnvifNetworkPanel_clear_details(OnvifNetworkPanel * self){
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (self);

    gtk_entry_set_text(GTK_ENTRY(priv->dhcp_dd),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->dhcp_dd, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->ip_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->ip_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->mask_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->mask_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->gateway_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->gateway_lbl, FALSE);

}

static void
OnvifNetworkPanel__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
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
                          GParamSpec *pspec)
{
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
OnvifNetworkPanel__class_init (OnvifNetworkPanelClass * klass)
{
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
OnvifNetworkPanel__init (OnvifNetworkPanel * self)
{
    OnvifNetworkPanelPrivate *priv = OnvifNetworkPanel__get_instance_private (self);
    priv->app = NULL;
    priv->device = NULL;
    OnvifNetworkPanel__create_ui(self);

    g_signal_connect (self, "map", G_CALLBACK (OnvifNetworkPanel__map_event_cb), self);
}

OnvifNetworkPanel * OnvifNetworkPanel__new(OnvifApp * app){
    if(!ONVIFMGR_IS_APP(app)){
        return NULL;
    }
    return g_object_new (ONVIFMGR_TYPE_NETWORKPANEL, "app", app, NULL);
}