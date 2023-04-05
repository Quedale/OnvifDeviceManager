#include "onvif_info.h"
#include "gui_utils.h"

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

void _update_details_page(void * user_data){
    char * hostname = NULL;
    OnvifDeviceInformation * dev_info = NULL;
    OnvifInterfaces * interfaces = NULL;
    OnvifScopes * scopes = NULL;

    OnvifInfo * self = (OnvifInfo *) user_data;
    if(!Device__addref(self->device) || !self->device->selected){
        goto exit;
    }

    hostname = OnvifDevice__device_getHostname(self->device->onvif_device);
    if(!Device__is_valid(self->device) || !self->device->selected)
        goto exit;
    dev_info = OnvifDevice__device_getDeviceInformation(self->device->onvif_device);
    if(!Device__is_valid(self->device) || !self->device->selected)
        goto exit;
    interfaces = OnvifDevice__device_getNetworkInterfaces(self->device->onvif_device);
    if(!Device__is_valid(self->device) || !self->device->selected)
        goto exit;
    scopes = OnvifDevice__device_getScopes(self->device->onvif_device);
    if(!Device__is_valid(self->device) || !self->device->selected)
        goto exit;

    gtk_entry_set_text(GTK_ENTRY(self->name_lbl),OnvifScopes__extract_scope(scopes,"name"));
    gtk_editable_set_editable  ((GtkEditable*)self->name_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->hostname_lbl),hostname);
    gtk_editable_set_editable  ((GtkEditable*)self->hostname_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->location_lbl),OnvifScopes__extract_scope(scopes,"location"));
    gtk_editable_set_editable  ((GtkEditable*)self->location_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->manufacturer_lbl),dev_info->manufacturer);
    gtk_editable_set_editable  ((GtkEditable*)self->manufacturer_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->model_lbl),dev_info->model);
    gtk_editable_set_editable  ((GtkEditable*)self->model_lbl, FALSE);
    
    gtk_entry_set_text(GTK_ENTRY(self->hardware_lbl),dev_info->hardwareId);
    gtk_editable_set_editable  ((GtkEditable*)self->hardware_lbl, FALSE);
    
    gtk_entry_set_text(GTK_ENTRY(self->firmware_lbl),dev_info->firmwareVersion);
    gtk_editable_set_editable  ((GtkEditable*)self->firmware_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->serial_lbl),dev_info->serialNumber);
    gtk_editable_set_editable  ((GtkEditable*)self->serial_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->ip_lbl),self->device->onvif_device->ip);
    gtk_editable_set_editable  ((GtkEditable*)self->ip_lbl, FALSE);

    int mac_count=0;
    for(int i=0;i<interfaces->count;i++){
        OnvifInterface * interface = interfaces->interfaces[i];
        if(interface->ipv4_enabled){
            for(int a=0;a<interface->ipv4_manual_count;a++){
                char * ip = interface->ipv4_manual[a];
                if(!strcmp(ip,self->device->onvif_device->ip)){
                    mac_count++;
                    char * format = "MAC Address #%i : ";
                    char buff[27];
                    snprintf(buff,sizeof(buff),format,mac_count);
                    GtkWidget * grid = gtk_grid_new();
                    GtkWidget * widget = gtk_label_new (buff);
                    gtk_widget_set_size_request(widget,130,-1);
                    gtk_label_set_xalign (GTK_LABEL(widget), 1.0);
                    gtk_grid_attach (GTK_GRID (grid), widget, 0, mac_count, 1, 1);
                    widget = gtk_entry_new();
                    gtk_widget_set_size_request(widget,300,-1);
                    gtk_entry_set_text(GTK_ENTRY(widget),interface->mac);
                    gtk_editable_set_editable  ((GtkEditable*)widget, FALSE);
                    gtk_grid_attach (GTK_GRID (grid), widget, 1, mac_count, 1, 1);
                    
                    gtk_box_pack_start (GTK_BOX(self->mac_lbl),grid,FALSE,FALSE,0);
                    gtk_widget_show_all(grid);
                }
            }
        }
    }
  
    gtk_entry_set_text(GTK_ENTRY(self->version_lbl),"SomeName");
    gtk_editable_set_editable  ((GtkEditable*)self->version_lbl, FALSE);

    gtk_entry_set_text(GTK_ENTRY(self->uri_lbl),self->device->onvif_device->device_soap->endpoint);
    gtk_editable_set_editable  ((GtkEditable*)self->uri_lbl, FALSE);

exit:
    if(hostname)
        free(hostname);
    OnvifDeviceInformation__destroy(dev_info);
    OnvifInterfaces__destroy(interfaces);
    OnvifScopes__destroy(scopes);
    Device__unref(self->device);
    if(self->done_callback)
        (*(self->done_callback))(self, self->done_user_data);
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