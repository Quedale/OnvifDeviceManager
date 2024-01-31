#include "onvif_info.h"
#include "gui_utils.h"
#include "clogger.h"

#define ONVIF_GET_HOSTNAME_ERROR "Error retrieving hostname"
#define ONVIF_GET_SCOPES_ERROR "Error retreiving scopes"
#define ONVIF_GET_INFO_ERROR "Error retrieving device information."
#define ONVIF_GET_NETWORK_ERROR "Error retreiving network details."

typedef struct _OnvifInfo {
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

    EventQueue * queue;
    GtkWidget * widget;
    Device * device;

    void (*done_callback)(OnvifInfo *, void * user_data);
    void * done_user_data;
} OnvifInfo;


typedef struct {
    OnvifInfo * info;
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

void OnvifInfo__create_ui(OnvifInfo * self){
    self->widget = gtk_scrolled_window_new (NULL, NULL);

    GtkWidget * grid = gtk_grid_new();

    int i = 0;
    self->name_lbl = add_label_entry(grid,i++,"Name : ");

    self->hostname_lbl = add_label_entry(grid,i++,"Hostname : ");

    self->location_lbl = add_label_entry(grid,i++,"Location : ");

    self->manufacturer_lbl = add_label_entry(grid,i++,"Manufacturer : ");

    self->model_lbl = add_label_entry(grid,i++,"Model : ");

    self->hardware_lbl = add_label_entry(grid,i++,"Hardware : ");

    self->firmware_lbl = add_label_entry(grid,i++,"Firmware : ");

    self->serial_lbl = add_label_entry(grid,i++,"Device ID : ");

    self->ip_lbl = add_label_entry(grid,i++,"IP Address : ");

    self->mac_lbl =  gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_grid_attach (GTK_GRID (grid), self->mac_lbl, 0, i++, 2, 1);

    self->version_lbl = add_label_entry(grid,i++,"ONVIF Version : ");

    self->uri_lbl = add_label_entry(grid,i++,"URI : ");

    gtk_container_add(GTK_CONTAINER(self->widget),grid);
}

OnvifInfo * OnvifInfo__create(EventQueue * queue){
    OnvifInfo *info  =  malloc(sizeof(OnvifInfo));
    info->queue = queue;
    info->device = NULL;
    info->done_callback = NULL;
    info->done_user_data = NULL;
    OnvifInfo__create_ui(info);
    return info;
}

void OnvifInfo__destroy(OnvifInfo* self){
    if(self){
        free(self);
    }
}

gboolean * onvif_info_gui_update (void * user_data){
    InfoGUIUpdate * update = (InfoGUIUpdate *) user_data;

    if(update->name) gtk_entry_set_text(GTK_ENTRY(update->info->name_lbl),update->name);
    gtk_editable_set_editable  ((GtkEditable*)update->info->name_lbl, FALSE);

    if(update->hostname) gtk_entry_set_text(GTK_ENTRY(update->info->hostname_lbl),update->hostname);
    gtk_editable_set_editable  ((GtkEditable*)update->info->hostname_lbl, FALSE);

    if(update->location) gtk_entry_set_text(GTK_ENTRY(update->info->location_lbl),update->location);
    gtk_editable_set_editable  ((GtkEditable*)update->info->location_lbl, FALSE);

    if(update->manufacturer) gtk_entry_set_text(GTK_ENTRY(update->info->manufacturer_lbl),update->manufacturer);
    gtk_editable_set_editable  ((GtkEditable*)update->info->manufacturer_lbl, FALSE);

    if(update->model) gtk_entry_set_text(GTK_ENTRY(update->info->model_lbl),update->model);
    gtk_editable_set_editable  ((GtkEditable*)update->info->model_lbl, FALSE);

    if(update->hardware) gtk_entry_set_text(GTK_ENTRY(update->info->hardware_lbl),update->hardware);
    gtk_editable_set_editable  ((GtkEditable*)update->info->hardware_lbl, FALSE);

    if(update->firmware) gtk_entry_set_text(GTK_ENTRY(update->info->firmware_lbl),update->firmware);
    gtk_editable_set_editable  ((GtkEditable*)update->info->firmware_lbl, FALSE);

    if(update->serial) gtk_entry_set_text(GTK_ENTRY(update->info->serial_lbl),update->serial);
    gtk_editable_set_editable  ((GtkEditable*)update->info->serial_lbl, FALSE);

    if(update->ip) gtk_entry_set_text(GTK_ENTRY(update->info->ip_lbl),update->ip);
    gtk_editable_set_editable  ((GtkEditable*)update->info->ip_lbl, FALSE);

    for(int i=0;i<update->mac_count;i++){
        char * format = "MAC Address #%i : ";
        char buff[27];
        snprintf(buff,sizeof(buff),format,i+1);
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

        gtk_box_pack_start (GTK_BOX(update->info->mac_lbl),grid,FALSE,FALSE,0);
        gtk_widget_show_all(grid);
    }

    if(update->version) gtk_entry_set_text(GTK_ENTRY(update->info->version_lbl),update->version);
    gtk_editable_set_editable  ((GtkEditable*)update->info->version_lbl, FALSE);

    if(update->uri) gtk_entry_set_text(GTK_ENTRY(update->info->uri_lbl),update->uri);
    gtk_editable_set_editable  ((GtkEditable*)update->info->uri_lbl, FALSE);

    if(update->info->done_callback)
        (*(update->info->done_callback))(update->info, update->info->done_user_data);

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
    return FALSE;
}

void _update_details_page(void * user_data){
    char * hostname = NULL;
    OnvifDeviceInformation * dev_info = NULL;
    OnvifInterfaces * interfaces = NULL;
    OnvifScopes * scopes = NULL;

    OnvifInfo * self = (OnvifInfo *) user_data;
    if(!CObject__addref((CObject*)self->device)){
        return;
    }

    if(!Device__is_selected(self->device)){
        goto exit;
    }

    int ginfo_success = 0;
    int gnetwork_success = 0;
    int gscopes_success = 0;
    OnvifDevice * onvif_device = Device__get_device(self->device);
    OnvifDeviceService * devserv = OnvifDevice__get_device_service(onvif_device);

    hostname = OnvifDeviceService__getHostname(devserv);
    if(!CObject__is_valid((CObject*)self->device) || !Device__is_selected(self->device))
        goto exit;

    dev_info = OnvifDeviceService__getDeviceInformation(devserv);
    if(OnvifDevice__get_last_error(onvif_device) == ONVIF_ERROR_NONE) ginfo_success = 1;
    if(!CObject__is_valid((CObject*)self->device) || !Device__is_selected(self->device))
        goto exit;

    interfaces = OnvifDeviceService__getNetworkInterfaces(devserv);
    if(OnvifDevice__get_last_error(onvif_device) == ONVIF_ERROR_NONE) gnetwork_success = 1;
    if(!CObject__is_valid((CObject*)self->device) || !Device__is_selected(self->device))
        goto exit;

    scopes = OnvifDeviceService__getScopes(devserv);
    if(OnvifDevice__get_last_error(onvif_device) == ONVIF_ERROR_NONE) gscopes_success = 1;
    if(!CObject__is_valid((CObject*)self->device) || !Device__is_selected(self->device))
        goto exit;

    InfoGUIUpdate * gui_update = malloc(sizeof(InfoGUIUpdate));
    gui_update->info = self;
    gui_update->mac_count = 0;
    gui_update->macs= malloc(0);
    gui_update->hostname = hostname; //No need to copy since OnvifDevice__device_getHostname returns malloc pointer

    if(gscopes_success){
        gui_update->name = OnvifScopes__extract_scope(scopes,"name"); //No need to copy since extract_scope returns a malloc pointer
        gui_update->location = OnvifScopes__extract_scope(scopes,"location");
    } else {
        gui_update->name = malloc(strlen(ONVIF_GET_SCOPES_ERROR)+1);
        strcpy(gui_update->name,ONVIF_GET_SCOPES_ERROR);
        gui_update->location = malloc(strlen(ONVIF_GET_SCOPES_ERROR)+1);
        strcpy(gui_update->location,ONVIF_GET_SCOPES_ERROR);
    }

    if(ginfo_success){
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
    } else {
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
    }

    gui_update->ip = OnvifDevice__get_host(onvif_device);

    if(gnetwork_success){
        for(int i=0;i<OnvifInterfaces__get_count(interfaces);i++){
            OnvifInterface * interface = OnvifInterfaces__get_interface(interfaces,i);
            if(OnvifInterface__is_ipv4_enabled(interface)){
                for(int a=0;a<OnvifInterface__get_ipv4_manual_count(interface);a++){
                    char * ip = OnvifInterface__get_ipv4_manual(interface)[a];
                    if(!strcmp(ip,gui_update->ip)){
                        gui_update->mac_count++;
                        gui_update->macs = realloc(gui_update->macs,sizeof(char *) * gui_update->mac_count);
                        gui_update->macs[gui_update->mac_count-1] = malloc(strlen(OnvifInterface__get_mac(interface))+1);
                        strcpy(gui_update->macs[gui_update->mac_count-1],OnvifInterface__get_mac(interface));
                    }
                }
            }
        }
    } else {
        gui_update->mac_count++;
        gui_update->macs = realloc(gui_update->macs,sizeof(char *) * gui_update->mac_count);
        gui_update->macs[gui_update->mac_count-1] = malloc(strlen(ONVIF_GET_NETWORK_ERROR)+1);
        strcpy(gui_update->macs[gui_update->mac_count-1],ONVIF_GET_NETWORK_ERROR);
    }

    gui_update->version = malloc(strlen("SomeName")+1);
    strcpy(gui_update->version,"SomeName");

    gui_update->uri = OnvifDeviceService__get_endpoint(devserv);

    gdk_threads_add_idle((void *)onvif_info_gui_update,gui_update);
exit:
    OnvifDeviceInformation__destroy(dev_info);
    OnvifInterfaces__destroy(interfaces);
    OnvifScopes__destroy(scopes);
    CObject__unref((CObject*)self->device);
}

void OnvifInfo_update_details(OnvifInfo * self, Device * device){
    self->device = device;
    EventQueue__insert(self->queue,_update_details_page,self);
}

void OnvifInfo_clear_details(OnvifInfo * self){
    gtk_entry_set_text(GTK_ENTRY(self->name_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->name_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->hostname_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->hostname_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->location_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->location_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->manufacturer_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->manufacturer_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->model_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->model_lbl, FALSE);
    
    gtk_entry_set_text(GTK_ENTRY(self->hardware_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->hardware_lbl, FALSE);
    
    gtk_entry_set_text(GTK_ENTRY(self->firmware_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->firmware_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->serial_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->serial_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->ip_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->ip_lbl, FALSE);

    gtk_container_foreach (GTK_CONTAINER (self->mac_lbl), (void*) gtk_widget_destroy, NULL);

    gtk_entry_set_text(GTK_ENTRY(self->version_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->version_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->uri_lbl),"");
    gtk_editable_set_editable  ((GtkEditable*)self->uri_lbl, FALSE);
}

void OnvifInfo__set_done_callback(OnvifInfo* self, void (*done_callback)(OnvifInfo *, void *), void * user_data){
    self->done_callback = done_callback;
    self->done_user_data = user_data;
}

GtkWidget * OnvifInfo__get_widget(OnvifInfo * self){
    return self->widget;
}