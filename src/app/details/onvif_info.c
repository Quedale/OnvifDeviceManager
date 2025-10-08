#include "onvif_info.h"
#include "../gui_utils.h"
#include "clogger.h"
#include "cstring_utils.h"

#define ONVIF_GET_HOSTNAME_ERROR "Error retrieving hostname"
#define ONVIF_GET_SCOPES_ERROR "Error retreiving scopes"
#define ONVIF_GET_INFO_ERROR "Error retrieving device information."
#define ONVIF_GET_NETWORK_ERROR "Error retreiving network details."
#define ONVIF_GET_DEVICE_CAPS_ERROR "Error retreiving device capabilities."

typedef struct {
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
} OnvifInfoPanelPrivate;


typedef struct {
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
} InfoGUIUpdate;

G_DEFINE_TYPE_WITH_PRIVATE(OnvifInfoPanel, OnvifInfoPanel_, ONVIFMGR_TYPE_DETAILSPANEL)

static void 
OnvifInfoPanel__createui(OnvifDetailsPanel * panel){
    OnvifInfoPanel * self = ONVIFMGR_INFOPANEL(panel);
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

static void 
OnvifInfoPanel__updateui (OnvifDetailsPanel * self, OnvifApp * app, OnvifMgrDeviceRow * device, void * user_data){
    InfoGUIUpdate * update = (InfoGUIUpdate *) user_data;

    OnvifInfoPanelPrivate *priv = OnvifInfoPanel__get_instance_private (ONVIFMGR_INFOPANEL(self));

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
    free(update);
}


static void * 
OnvifInfoPanel__getdata(OnvifDetailsPanel * self, OnvifApp * app, OnvifMgrDeviceRow * device, QueueEvent * qevt){
    OnvifDeviceHostnameInfo * hostname = NULL;
    OnvifDeviceInformation * dev_info = NULL;
    OnvifDeviceInterfaces * interfaces = NULL;
    OnvifScopes * scopes = NULL;
    OnvifCapabilities* caps = NULL;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifMgrDeviceRow__is_selected(device)){
        C_TRAIL("OnvifInfoPanel__getdata - invalid device or no longer selected.");
        return NULL;
    }

    InfoGUIUpdate * gui_update = NULL;
    OnvifDevice * onvif_device = OnvifMgrDeviceRow__get_device(device);
    OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_device);

    hostname = OnvifDeviceService__getHostname(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifMgrDeviceRow__is_selected(device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    dev_info = OnvifDeviceService__getDeviceInformation(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifMgrDeviceRow__is_selected(device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    interfaces = OnvifDeviceService__getNetworkInterfaces(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifMgrDeviceRow__is_selected(device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    scopes = OnvifDeviceService__getScopes(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifMgrDeviceRow__is_selected(device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    caps = OnvifDeviceService__getCapabilities(devserv);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(device) || !OnvifMgrDeviceRow__is_selected(device) || QueueEvent__is_cancelled(qevt))
        goto exit;

    //Initialize idle update data struct
    gui_update = malloc(sizeof(InfoGUIUpdate));
    gui_update->mac_count = 0;
    gui_update->macs= NULL;

    //GetHostname fault handling
    SoapFault * fault = SoapObject__get_fault(SOAP_OBJECT(hostname));
    switch(*fault){
        case SOAP_FAULT_NONE:
            cstring_safe_copy(&gui_update->hostname, OnvifDeviceHostnameInfo__get_name(hostname), NULL);
            break;
        case SOAP_FAULT_CONNECTION_ERROR:
            cstring_safe_copy(&gui_update->hostname, "Error: Failed to connect...", NULL);
            break;
        case SOAP_FAULT_NOT_VALID:
            cstring_safe_copy(&gui_update->hostname, "Error: Not a valid ONVIF response...", NULL);
            break;
        case SOAP_FAULT_UNAUTHORIZED:
            cstring_safe_copy(&gui_update->hostname, "Error: Unauthorized...", NULL);
            break;
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
            cstring_safe_copy(&gui_update->hostname, "Error: Action Not Supported...", NULL);
            break;
        case SOAP_FAULT_UNEXPECTED:
        default:
            cstring_safe_copy(&gui_update->hostname, ONVIF_GET_HOSTNAME_ERROR, NULL);
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
            cstring_safe_copy(&gui_update->name, ONVIF_GET_SCOPES_ERROR, NULL);
            cstring_safe_copy(&gui_update->location, ONVIF_GET_SCOPES_ERROR, NULL);
            break;
    }

    //getDeviceInformation fault handling
    fault = SoapObject__get_fault(SOAP_OBJECT(dev_info));
    switch(*fault){
        case SOAP_FAULT_NONE:
            cstring_safe_copy(&gui_update->manufacturer, OnvifDeviceInformation__get_manufacturer(dev_info), NULL);
            cstring_safe_copy(&gui_update->model, OnvifDeviceInformation__get_model(dev_info), NULL);
            cstring_safe_copy(&gui_update->hardware, OnvifDeviceInformation__get_hardwareId(dev_info), NULL);
            cstring_safe_copy(&gui_update->firmware, OnvifDeviceInformation__get_firmwareVersion(dev_info), NULL);
            cstring_safe_copy(&gui_update->serial, OnvifDeviceInformation__get_serialNumber(dev_info), NULL);
            break;
        case SOAP_FAULT_CONNECTION_ERROR:
        case SOAP_FAULT_NOT_VALID:
        case SOAP_FAULT_UNAUTHORIZED:
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
        case SOAP_FAULT_UNEXPECTED:
        default:
            cstring_safe_copy(&gui_update->manufacturer, ONVIF_GET_INFO_ERROR, NULL);
            cstring_safe_copy(&gui_update->model, ONVIF_GET_INFO_ERROR, NULL);
            cstring_safe_copy(&gui_update->hardware, ONVIF_GET_INFO_ERROR, NULL);
            cstring_safe_copy(&gui_update->firmware, ONVIF_GET_INFO_ERROR, NULL);
            cstring_safe_copy(&gui_update->serial, ONVIF_GET_INFO_ERROR, NULL);
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

                cstring_safe_copy(&gui_update->macs[gui_update->mac_count-1], OnvifDeviceInterface__get_mac(interface), "MAC Address not defined.");
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
            cstring_safe_copy(&gui_update->macs[gui_update->mac_count-1], ONVIF_GET_NETWORK_ERROR, NULL);
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
            cstring_safe_copy(&gui_update->version, ONVIF_GET_DEVICE_CAPS_ERROR, NULL);
            break;
    }

    gui_update->uri = OnvifBaseService__get_endpoint(ONVIF_BASE_SERVICE(devserv));
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

    return gui_update;
}

static void 
OnvifInfoPanel_clearui(OnvifDetailsPanel * panel){
    OnvifInfoPanel * self = ONVIFMGR_INFOPANEL(panel);
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
OnvifInfoPanel__class_init (OnvifInfoPanelClass * klass){
    OnvifDetailsPanelClass * detailsp_klass = ONVIFMGR_DETAILSPANEL_CLASS(klass);
    detailsp_klass->getdata = OnvifInfoPanel__getdata;
    detailsp_klass->updateui = OnvifInfoPanel__updateui;
    detailsp_klass->clearui = OnvifInfoPanel_clearui;
    detailsp_klass->createui = OnvifInfoPanel__createui;
}

static void
OnvifInfoPanel__init (OnvifInfoPanel * self){

}

OnvifInfoPanel * OnvifInfoPanel__new(OnvifApp * app){
    if(!ONVIFMGR_IS_APP(app)){
        return NULL;
    }
    return g_object_new (ONVIFMGR_TYPE_INFOPANEL, "app", app, NULL);
}