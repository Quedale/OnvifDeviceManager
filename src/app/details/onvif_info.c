#include "onvif_info.h"
#include "../gui_utils.h"
#include "clogger.h"

#define ONVIF_GET_HOSTNAME_ERROR "Error retrieving hostname"
#define ONVIF_GET_SCOPES_ERROR "Error retreiving scopes"
#define ONVIF_GET_INFO_ERROR "Error retrieving device information."
#define ONVIF_GET_NETWORK_ERROR "Error retreiving network details."
#define ONVIF_GET_DEVICE_CAPS_ERROR "Error retreiving device capabilities."

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
    GtkWidget * info_page;
    GtkWidget * name_lbl;
    GtkWidget * hostname_lbl;
    GtkWidget * location_lbl;
    GtkWidget * manufacturer_lbl;
    GtkWidget * model_lbl;
    GtkWidget * hardware_lbl;
    GtkWidget * firmware_lbl;
    GtkWidget * serial_lbl;
    GtkWidget * ip_lbl;
    GtkWidget * mac_lbl;
    GtkWidget * version_lbl;
    GtkWidget * uri_lbl;

    QueueEvent * previous_event;
    gulong event_signal;
} OnvifInfoPanelPrivate;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

typedef struct {
    OnvifMgrDeviceRow * device;
    OnvifInfoPanel * info;
    char * name;
    char * hostname;
    char * location;
    char * manufacturer;
    char * model;
    char * hardware;
    char * firmware;
    char * serial;
    char * ip;
    int mac_count;
    char ** macs;
    char * version;
    char * uri;
    QueueEvent * qevt; //Keep reference to check iscancelled before rendering result

} InfoGUIUpdate;

typedef struct {
    OnvifInfoPanel * info;
    OnvifMgrDeviceRow * device;
} InfoDataUpdate;

G_DEFINE_TYPE_WITH_PRIVATE(OnvifInfoPanel, OnvifInfoPanel_, GTK_TYPE_SCROLLED_WINDOW)

void OnvifInfoPanel_update_details(OnvifInfoPanel * self);
void OnvifInfoPanel_clear_details(OnvifInfoPanel * self);

void OnvifInfoPanel__create_ui(OnvifInfoPanel * self){

    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (self);

    GtkWidget * grid = gtk_grid_new();

    int i = 0;
    priv->name_lbl = add_label_entry(grid,i++,"Name : ");

    priv->hostname_lbl = add_label_entry(grid,i++,"Hostname : ");

    priv->location_lbl = add_label_entry(grid,i++,"Location : ");

    priv->manufacturer_lbl = add_label_entry(grid,i++,"Manufacturer : ");

    priv->model_lbl = add_label_entry(grid,i++,"Model : ");

    priv->hardware_lbl = add_label_entry(grid,i++,"Hardware : ");

    priv->firmware_lbl = add_label_entry(grid,i++,"Firmware : ");

    priv->serial_lbl = add_label_entry(grid,i++,"Device ID : ");

    priv->ip_lbl = add_label_entry(grid,i++,"IP Address : ");

    priv->mac_lbl =  gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_grid_attach (GTK_GRID (grid), priv->mac_lbl, 0, i++, 2, 1);

    priv->version_lbl = add_label_entry(grid,i++,"ONVIF Version : ");

    priv->uri_lbl = add_label_entry(grid,i++,"URI : ");

    gtk_container_add(GTK_CONTAINER(self),grid);
}

void OnvifInfoPanel__device_changed_cb(OnvifApp * app, OnvifMgrDeviceRow * device, OnvifInfoPanel * info){
    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (info);
    priv->device = device;
    OnvifInfoPanel_clear_details(info);
    if(gtk_widget_get_mapped(GTK_WIDGET(info)) && ONVIFMGR_IS_DEVICEROW(device) && OnvifMgrDeviceRow__is_initialized(device)){
        OnvifInfoPanel_update_details(info);
    }
}

void OnvifInfoPanel__map_event_cb (GtkWidget* self, OnvifInfoPanel * info){
    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (info);
    if(gtk_widget_get_mapped(GTK_WIDGET(info)) && ONVIFMGR_IS_DEVICEROW(priv->device) && OnvifMgrDeviceRow__is_initialized(priv->device)){
        OnvifInfoPanel_clear_details(info);
        OnvifInfoPanel_update_details(info);
    }
}

gboolean onvif_info_gui_update (void * user_data){
    InfoGUIUpdate * update = (InfoGUIUpdate *) user_data;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(update->device) || !OnvifMgrDeviceRow__is_selected(update->device) || QueueEvent__is_cancelled(update->qevt)){
        goto exit;
    }

    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (update->info);

    if(update->name) gtk_entry_set_text(GTK_ENTRY(priv->name_lbl),update->name);
    gtk_editable_set_editable  ((GtkEditable*)priv->name_lbl, FALSE);

    if(update->hostname) gtk_entry_set_text(GTK_ENTRY(priv->hostname_lbl),update->hostname);
    gtk_editable_set_editable  ((GtkEditable*)priv->hostname_lbl, FALSE);

    if(update->location) gtk_entry_set_text(GTK_ENTRY(priv->location_lbl),update->location);
    gtk_editable_set_editable  ((GtkEditable*)priv->location_lbl, FALSE);

    if(update->manufacturer) gtk_entry_set_text(GTK_ENTRY(priv->manufacturer_lbl),update->manufacturer);
    gtk_editable_set_editable  ((GtkEditable*)priv->manufacturer_lbl, FALSE);

    if(update->model) gtk_entry_set_text(GTK_ENTRY(priv->model_lbl),update->model);
    gtk_editable_set_editable  ((GtkEditable*)priv->model_lbl, FALSE);

    if(update->hardware) gtk_entry_set_text(GTK_ENTRY(priv->hardware_lbl),update->hardware);
    gtk_editable_set_editable  ((GtkEditable*)priv->hardware_lbl, FALSE);

    if(update->firmware) gtk_entry_set_text(GTK_ENTRY(priv->firmware_lbl),update->firmware);
    gtk_editable_set_editable  ((GtkEditable*)priv->firmware_lbl, FALSE);

    if(update->serial) gtk_entry_set_text(GTK_ENTRY(priv->serial_lbl),update->serial);
    gtk_editable_set_editable  ((GtkEditable*)priv->serial_lbl, FALSE);

    if(update->ip) gtk_entry_set_text(GTK_ENTRY(priv->ip_lbl),update->ip);
    gtk_editable_set_editable  ((GtkEditable*)priv->ip_lbl, FALSE);

    for(int i=0;i<update->mac_count;i++){
        char buff[27];
        snprintf(buff,sizeof(buff),"MAC Address #%i : ",i+1);
        GtkWidget * grid = gtk_grid_new();
        GtkWidget * widget = gtk_label_new (buff);
        gtk_widget_set_size_request(widget,130,-1);
        gtk_label_set_xalign (GTK_LABEL(widget), 1.0);
        gtk_grid_attach (GTK_GRID (grid), widget, 0, i+1, 1, 1);
        widget = gtk_entry_new();
        gtk_widget_set_size_request(widget,300,-1);
        gtk_entry_set_text(GTK_ENTRY(widget),update->macs[i]);
        gtk_editable_set_editable  ((GtkEditable*)widget, FALSE);
        gtk_grid_attach (GTK_GRID (grid), widget, 1, i+1, 1, 1);

        gtk_box_pack_start (GTK_BOX(priv->mac_lbl),grid,FALSE,FALSE,0);
        gtk_widget_show_all(grid);
    }

    if(update->version) gtk_entry_set_text(GTK_ENTRY(priv->version_lbl),update->version);
    gtk_editable_set_editable  ((GtkEditable*)priv->version_lbl, FALSE);

    if(update->uri) gtk_entry_set_text(GTK_ENTRY(priv->uri_lbl),update->uri);
    gtk_editable_set_editable  ((GtkEditable*)priv->uri_lbl, FALSE);

    g_signal_emit (update->info, signals[FINISHED], 0, update->device);
exit:
    free(update->name);
    free(update->hostname);
    free(update->location);
    free(update->manufacturer);
    free(update->model);
    free(update->hardware);
    free(update->firmware);
    free(update->serial);
    free(update->ip);
    for(int i=0;i<update->mac_count;i++){
        free(update->macs[i]);
    }
    free(update->macs);
    free(update->version);
    free(update->uri);
    g_object_unref(update->device);
    g_object_unref(update->qevt);
    free(update);
    return FALSE;
}

void update_details_event_cancelled_cb(QueueEvent * qevt, void * user_data){
    InfoDataUpdate * input = (InfoDataUpdate *) user_data;
    gui_signal_emit(input->info, signals[FINISHED], input->device);
}

void _update_details_page_cleanup(QueueEvent * qevt, int cancelled, void * user_data){
    InfoDataUpdate * input = (InfoDataUpdate *) user_data;
    g_object_unref(input->device);
    free(input);
}

void _update_details_page(QueueEvent * qevt, void * user_data){
    OnvifDeviceHostnameInfo * hostname = NULL;
    OnvifDeviceInformation * dev_info = NULL;
    OnvifDeviceInterfaces * interfaces = NULL;
    OnvifScopes * scopes = NULL;
    OnvifCapabilities* caps = NULL;

    InfoDataUpdate * input = (InfoDataUpdate *) user_data;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(input->device)){
        C_TRAIL("_update_details_page - invalid device.");
        return;
    }

    if(!OnvifMgrDeviceRow__is_selected(input->device)){
        goto exit;
    }

    OnvifDevice * onvif_device = OnvifMgrDeviceRow__get_device(input->device);
    OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_device);

    hostname = OnvifDeviceService__getHostname(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(input->device) || !OnvifMgrDeviceRow__is_selected(input->device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    dev_info = OnvifDeviceService__getDeviceInformation(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(input->device) || !OnvifMgrDeviceRow__is_selected(input->device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    interfaces = OnvifDeviceService__getNetworkInterfaces(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(input->device) || !OnvifMgrDeviceRow__is_selected(input->device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    scopes = OnvifDeviceService__getScopes(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(input->device) || !OnvifMgrDeviceRow__is_selected(input->device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    caps = OnvifDeviceService__getCapabilities(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(input->device) || !OnvifMgrDeviceRow__is_selected(input->device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    //Initialize idle update data struct
    InfoGUIUpdate * gui_update = malloc(sizeof(InfoGUIUpdate));
    gui_update->info = input->info;
    gui_update->device = input->device;
    gui_update->mac_count = 0;
    gui_update->macs= NULL;
    gui_update->qevt = qevt;

    //GetHostname fault handling
    SoapFault * fault = SoapObject__get_fault(SOAP_OBJECT(hostname));
    switch(*fault){
        case SOAP_FAULT_NONE:
            gui_update->hostname = malloc(strlen(OnvifDeviceHostnameInfo__get_name(hostname))+1);
            strcpy(gui_update->hostname,OnvifDeviceHostnameInfo__get_name(hostname));
            break;
        case SOAP_FAULT_CONNECTION_ERROR:
            gui_update->hostname = malloc(strlen("Error: Failed to connect...")+1);
            strcpy(gui_update->hostname,"Error: Failed to connect...");
            break;
        case SOAP_FAULT_NOT_VALID:
            gui_update->hostname = malloc(strlen("Error: Not a valid ONVIF response...")+1);
            strcpy(gui_update->hostname,"Error: Not a valid ONVIF response...");
            break;
        case SOAP_FAULT_UNAUTHORIZED:
            gui_update->hostname = malloc(strlen("Error: Unauthorized...")+1);
            strcpy(gui_update->hostname,"Error: Unauthorized...");
            break;
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
            gui_update->hostname = malloc(strlen("Error: Action Not Supported...")+1);
            strcpy(gui_update->hostname,"Error: Action Not Supported...");
            break;
        case SOAP_FAULT_UNEXPECTED:
        default:
            gui_update->hostname = malloc(strlen(ONVIF_GET_HOSTNAME_ERROR)+1);
            strcpy(gui_update->hostname,ONVIF_GET_HOSTNAME_ERROR);
            break;
    }

    //GetScopes fault handling
    fault = SoapObject__get_fault(SOAP_OBJECT(scopes));
    switch(*fault){
        case SOAP_FAULT_NONE:
            gui_update->name = OnvifScopes__extract_scope(scopes,"name"); //No need to copy since extract_scope returns a malloc pointer
            gui_update->location = OnvifScopes__extract_scope(scopes,"location");
            break;
        case SOAP_FAULT_CONNECTION_ERROR:
        case SOAP_FAULT_NOT_VALID:
        case SOAP_FAULT_UNAUTHORIZED:
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
        case SOAP_FAULT_UNEXPECTED:
        default:
            gui_update->name = malloc(strlen(ONVIF_GET_SCOPES_ERROR)+1);
            strcpy(gui_update->name,ONVIF_GET_SCOPES_ERROR);
            gui_update->location = malloc(strlen(ONVIF_GET_SCOPES_ERROR)+1);
            strcpy(gui_update->location,ONVIF_GET_SCOPES_ERROR);
            break;
    }

    //getDeviceInformation fault handling
    fault = SoapObject__get_fault(SOAP_OBJECT(dev_info));
    switch(*fault){
        case SOAP_FAULT_NONE:
            gui_update->manufacturer = malloc(strlen(OnvifDeviceInformation__get_manufacturer(dev_info))+1);
            strcpy(gui_update->manufacturer,OnvifDeviceInformation__get_manufacturer(dev_info));

            gui_update->model = malloc(strlen(OnvifDeviceInformation__get_model(dev_info))+1);
            strcpy(gui_update->model,OnvifDeviceInformation__get_model(dev_info));

            gui_update->hardware = malloc(strlen(OnvifDeviceInformation__get_hardwareId(dev_info))+1);
            strcpy(gui_update->hardware,OnvifDeviceInformation__get_hardwareId(dev_info));

            gui_update->firmware = malloc(strlen(OnvifDeviceInformation__get_firmwareVersion(dev_info))+1);
            strcpy(gui_update->firmware,OnvifDeviceInformation__get_firmwareVersion(dev_info));

            gui_update->serial = malloc(strlen(OnvifDeviceInformation__get_serialNumber(dev_info))+1);
            strcpy(gui_update->serial,OnvifDeviceInformation__get_serialNumber(dev_info));
            break;
        case SOAP_FAULT_CONNECTION_ERROR:
        case SOAP_FAULT_NOT_VALID:
        case SOAP_FAULT_UNAUTHORIZED:
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
        case SOAP_FAULT_UNEXPECTED:
        default:
            gui_update->manufacturer = malloc(strlen(ONVIF_GET_INFO_ERROR)+1);
            strcpy(gui_update->manufacturer,ONVIF_GET_INFO_ERROR);
            gui_update->model = malloc(strlen(ONVIF_GET_INFO_ERROR)+1);
            strcpy(gui_update->model,ONVIF_GET_INFO_ERROR);
            gui_update->hardware = malloc(strlen(ONVIF_GET_INFO_ERROR)+1);
            strcpy(gui_update->hardware,ONVIF_GET_INFO_ERROR);
            gui_update->firmware = malloc(strlen(ONVIF_GET_INFO_ERROR)+1);
            strcpy(gui_update->firmware,ONVIF_GET_INFO_ERROR);
            gui_update->serial = malloc(strlen(ONVIF_GET_INFO_ERROR)+1);
            strcpy(gui_update->serial,ONVIF_GET_INFO_ERROR);
            break;
    }

    gui_update->ip = OnvifDevice__get_host(onvif_device);

    //getDeviceInterfaces fault handling
    fault = SoapObject__get_fault(SOAP_OBJECT(interfaces));
    switch(*fault){
        case SOAP_FAULT_NONE:
            for(int i=0;i<OnvifDeviceInterfaces__get_count(interfaces);i++){
                OnvifDeviceInterface * interface = OnvifDeviceInterfaces__get_interface(interfaces,i);
                gui_update->mac_count++;
                if(gui_update->macs){
                    gui_update->macs = realloc(gui_update->macs,sizeof(char *) * gui_update->mac_count);
                } else {
                    gui_update->macs = malloc(sizeof(char *) * gui_update->mac_count);
                }
                if(OnvifDeviceInterface__get_mac(interface)){
                    gui_update->macs[gui_update->mac_count-1] = malloc(strlen(OnvifDeviceInterface__get_mac(interface))+1);
                    strcpy(gui_update->macs[gui_update->mac_count-1],OnvifDeviceInterface__get_mac(interface));
                } else {
                    gui_update->macs[gui_update->mac_count-1] = malloc(strlen("MAC Address not defined.")+1);
                    strcpy(gui_update->macs[gui_update->mac_count-1],"MAC Address not defined.");
                }

            }
            break;
        case SOAP_FAULT_CONNECTION_ERROR:
        case SOAP_FAULT_NOT_VALID:
        case SOAP_FAULT_UNAUTHORIZED:
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
        case SOAP_FAULT_UNEXPECTED:
        default:
            gui_update->mac_count++;
            gui_update->macs = realloc(gui_update->macs,sizeof(char *) * gui_update->mac_count);
            gui_update->macs[gui_update->mac_count-1] = malloc(strlen(ONVIF_GET_NETWORK_ERROR)+1);
            strcpy(gui_update->macs[gui_update->mac_count-1],ONVIF_GET_NETWORK_ERROR);
            break;
    }
    
    OnvifSystemCapabilities * sys_caps = NULL;
    //GetCapabilities fault handling
    fault = SoapObject__get_fault(SOAP_OBJECT(caps));
    switch(*fault){
        case SOAP_FAULT_NONE:
            sys_caps = OnvifDeviceCapabilities__get_system(OnvifCapabilities__get_device(caps));
            gui_update->version = malloc(6);
            sprintf(gui_update->version, "%d.%02d", OnvifSystemCapabilities__get_majorVersion(sys_caps),OnvifSystemCapabilities__get_minorVersion(sys_caps));
            break;
        case SOAP_FAULT_CONNECTION_ERROR:
        case SOAP_FAULT_NOT_VALID:
        case SOAP_FAULT_UNAUTHORIZED:
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
        case SOAP_FAULT_UNEXPECTED:
        default:
            gui_update->version = malloc(strlen(ONVIF_GET_DEVICE_CAPS_ERROR)+1);
            strcpy(gui_update->version,ONVIF_GET_DEVICE_CAPS_ERROR);
            break;
    }

    gui_update->uri = OnvifBaseService__get_endpoint(ONVIF_BASE_SERVICE(devserv));
    g_object_ref(gui_update->device);
    g_object_ref(gui_update->qevt);
    gdk_threads_add_idle(G_SOURCE_FUNC(onvif_info_gui_update),gui_update);
exit:
    if(hostname)
        g_object_unref(hostname);
    if(dev_info)
        g_object_unref(dev_info);
    if(interfaces)
        g_object_unref(interfaces);
    if(scopes)
        g_object_unref(scopes);
    if(caps)
        g_object_unref(caps);
}

void OnvifInfoPanel_update_details(OnvifInfoPanel * self){
    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (self);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(priv->device)){
        C_TRAIL("update_details_priv - invalid device.");
        return;
    }
    
    OnvifDevice * odev = OnvifMgrDeviceRow__get_device(priv->device);
    if(!OnvifDevice__is_authenticated(odev)){
        return;
    }

    InfoDataUpdate * input = malloc(sizeof(InfoDataUpdate));
    input->device = priv->device;
    input->info = self;
    g_object_ref(input->device);
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
    priv->previous_event = EventQueue__insert_plain(OnvifApp__get_EventQueue(priv->app), input->device, _update_details_page,input, _update_details_page_cleanup);
    priv->event_signal = g_signal_connect (priv->previous_event, "cancelled", G_CALLBACK (update_details_event_cancelled_cb), input);
    g_object_ref(priv->previous_event);
}

void OnvifInfoPanel_clear_details(OnvifInfoPanel * self){
    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (self);

    gtk_entry_set_text(GTK_ENTRY(priv->name_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->name_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->hostname_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->hostname_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->location_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->location_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->manufacturer_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->manufacturer_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->model_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->model_lbl, FALSE);
    
    gtk_entry_set_text(GTK_ENTRY(priv->hardware_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->hardware_lbl, FALSE);
    
    gtk_entry_set_text(GTK_ENTRY(priv->firmware_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->firmware_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->serial_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->serial_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->ip_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->ip_lbl, FALSE);

    gtk_container_foreach (GTK_CONTAINER (priv->mac_lbl), (GtkCallback)gui_widget_destroy, NULL);

    gtk_entry_set_text(GTK_ENTRY(priv->version_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->version_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(priv->uri_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)priv->uri_lbl, FALSE);
}

static void
OnvifInfoPanel__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    OnvifInfoPanel * info = ONVIFMGR_INFOPANEL (object);
    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (info);
    switch (prop_id){
        case PROP_APP:
            priv->app = ONVIFMGR_APP(g_value_get_object (value));
            g_signal_connect (priv->app, "device-changed", G_CALLBACK (OnvifInfoPanel__device_changed_cb), info);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifInfoPanel__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    OnvifInfoPanel *info = ONVIFMGR_INFOPANEL (object);
    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (info);
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
OnvifInfoPanel__class_init (OnvifInfoPanelClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = OnvifInfoPanel__set_property;
    object_class->get_property = OnvifInfoPanel__get_property;

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
OnvifInfoPanel__init (OnvifInfoPanel * self)
{
    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (self);
    priv->app = NULL;
    priv->device = NULL;
    OnvifInfoPanel__create_ui(self);

    g_signal_connect (self, "map", G_CALLBACK (OnvifInfoPanel__map_event_cb), self);
}

OnvifInfoPanel * OnvifInfoPanel__new(OnvifApp * app){
    if(!ONVIFMGR_IS_APP(app)){
        return NULL;
    }
    return g_object_new (ONVIFMGR_TYPE_INFOPANEL, "app", app, NULL);
}