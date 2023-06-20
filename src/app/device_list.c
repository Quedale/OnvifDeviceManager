
#include <stddef.h>
#include <stdio.h>
#include "client.h"
#include "device_list.h"
#include "gui_utils.h"
#include <netdb.h>
#include <arpa/inet.h>

extern char _binary_prohibited_icon_png_size[];
extern char _binary_prohibited_icon_png_start[];
extern char _binary_prohibited_icon_png_end[];

extern char _binary_locked_icon_png_size[];
extern char _binary_locked_icon_png_start[];
extern char _binary_locked_icon_png_end[];

extern char _binary_warning_png_size[];
extern char _binary_warning_png_start[];
extern char _binary_warning_png_end[];

void DeviceList__init(DeviceList* self) {
    self->devices=malloc(0);
    self->device_count=0;
}

DeviceList* DeviceList__create() {
   DeviceList* result = (DeviceList*) malloc(sizeof(DeviceList));
   DeviceList__init(result);
   return result;
}

void DeviceList__destroy(DeviceList* self) {
  if (self) {
     DeviceList__clear(self);
     free(self->devices);
     free(self);
  }
}

Device ** DeviceList__remove_element_and_shift(DeviceList* self, Device **array, int index, int array_length)
{
    int i;
    for(i = index; i < array_length; i++) {
        array[i] = array[i + 1];
    }
    return array;
};

void DeviceList__insert_element(DeviceList* self, Device * record, int index)
{ 
    int i;
    int count = self->device_count;
    self->devices = realloc (self->devices,sizeof (Device) * (count+1));
    for(i=self->device_count; i> index; i--){
        self->devices[i] = self->devices[i-1];
    }
    self->devices[index]=record;
    self->device_count++;
};

void DeviceList__clear(DeviceList* self){
    int i;
    for(i=0; i < self->device_count; i++){
        Device__destroy(self->devices[i]);
    }
    self->device_count = 0;
    self->devices = realloc(self->devices,0);
}

void DeviceList__remove_element(DeviceList* self, int index){
    //Remove element and shift content
    self->devices = DeviceList__remove_element_and_shift(self,self->devices, index, self->device_count);  /* First shift the elements, then reallocate */
    //Resize count
    self->device_count--;
    //Assign arythmatic
    int count = self->device_count; 
    //Resize array memory
    self->devices = realloc (self->devices,sizeof(Device) * count);
};


void Device__init(Device* self, OnvifDevice * onvif_device) {
    self->onvif_device = onvif_device;
    self->refcount = 1;
    self->destroyed = 0;
    self->selected=0;
    self->profile_index=0;
    self->ref_lock =malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(self->ref_lock, NULL);
}

Device * Device__create(OnvifDevice * onvif_device){
    Device* result = (Device*) malloc(sizeof(Device));
    Device__init(result, onvif_device);
    return result;
}

void Device__destroy(Device* self) {
  if (self && self->refcount == 0) {
     OnvifDevice__destroy(self->onvif_device);
     pthread_mutex_destroy(self->ref_lock);
     free(self->ref_lock);
     free(self);
  } else if(self){
    self->destroyed=1;
    Device__unref(self);
  }
}

int Device__addref(Device *self){
    if(Device__is_valid(self)){
        pthread_mutex_lock(self->ref_lock);
        self->refcount++;
        pthread_mutex_unlock(self->ref_lock);
        return 1;
    }

    return 0;
}

void Device__unref(Device *self){
    if(self){
        pthread_mutex_lock(self->ref_lock);
        self->refcount--;
        pthread_mutex_unlock(self->ref_lock);
        if(self->refcount == 0){
            Device__destroy(self);
        }
    }
}

int Device__is_valid(Device* device){
    if(device && device->refcount > 0 && !device->destroyed){
        return 1;
    }

    return 0;
}

void _lookup_hostname(void * user_data){
    Device * device = (Device *) user_data;
    char * hostname;

    if(!Device__addref(device)){
        goto exit;
    }

    printf("NSLookup ... %s\n",device->onvif_device->ip);
    //Lookup hostname
    struct in_addr in_a;
    inet_pton(AF_INET, device->onvif_device->ip, &in_a);
    struct hostent* host;
    host = gethostbyaddr( (const void*)&in_a, 
                        sizeof(struct in_addr), 
                        AF_INET );
    if(host){
        printf("Found hostname : %s\n",host->h_name);
        hostname = host->h_name;
    } else {
        printf("Failed to get hostname ...\n");
        hostname = NULL;
    }

    printf("Retrieved hostname : %s\n",hostname);

exit:
    Device__unref(device);
}


void Device__lookup_hostname(Device* device, EventQueue * queue){
    EventQueue__insert(queue,_lookup_hostname,device);
}

void _load_thumbnail(void * user_data){
    GtkWidget *image;
    GError *error = NULL;
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *scaled_pixbuf = NULL;
    double size;
    char * imgdata = NULL;
    int freeimgdata = 0;
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();

    Device * device = (Device *) user_data;

    if(!Device__addref(device)){
        goto exit;
    }

    if(device->onvif_device->authorized){
        //TODO handle profiles
        struct chunk * imgchunk = OnvifDevice__media_getSnapshot(device->onvif_device,device->profile_index);
        if(!imgchunk){
            //TODO Set error image
            printf("Error retrieve snapshot.");
            goto warning;
        }
        imgdata = imgchunk->buffer;
        size = imgchunk->size;
        freeimgdata = 1;
        free(imgchunk);
    } else {
        imgdata = _binary_locked_icon_png_start;
        size = _binary_locked_icon_png_end - _binary_locked_icon_png_start;
    }

    //Check is device is still valid. (User performed scan before snapshot finished)
    if(!Device__is_valid(device)){
        goto exit;
    }

    //Attempt to get downloaded pixbuf or locked icon
    if(gdk_pixbuf_loader_write (loader, (unsigned char *)imgdata, size,&error)){
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    } else {
        if(error->message){
            printf("Error writing png to GtkPixbufLoader : %s\n",error->message);
        } else {
            printf("Error writing png to GtkPixbufLoader : [null]\n");
        }
    }
  
warning:
    //Check is device is still valid. (User performed scan before snapshot finished)
    if(!Device__is_valid(device)){
        goto exit;
    }

    //Show warning icon in case of failure
    if(!pixbuf){
        if(gdk_pixbuf_loader_write (loader, (unsigned char *)_binary_warning_png_start, _binary_warning_png_end - _binary_warning_png_start,&error)){
            pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        } else {
            if(error->message){
                printf("Error writing warning png to GtkPixbufLoader : %s\n",error->message);
            } else {
                printf("Error writing warning png to GtkPixbufLoader : [null]\n");
            }
        }
    }

    //Check is device is still valid. (User performed scan before spinner showed)
    if(!Device__is_valid(device)){
        goto exit;
    }

    //Display pixbuf
    if(pixbuf){
        double ph = gdk_pixbuf_get_height (pixbuf);
        double pw = gdk_pixbuf_get_width (pixbuf);
        double newpw = 40 / ph * pw;
        scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,newpw,40,GDK_INTERP_NEAREST);
        image = gtk_image_new_from_pixbuf (scaled_pixbuf);

        //Check is device is still valid. (User performed scan before scale finished)
        if(!Device__is_valid(device)){
            goto exit;
        }

        gui_update_widget_image(image,device->image_handle);
    } else {
        printf("Failed all thumbnail creation.\n");
    }

exit:
    if(loader){
        gdk_pixbuf_loader_close(loader,NULL);
        g_object_unref(loader);
    }
    if(scaled_pixbuf){
        g_object_unref(scaled_pixbuf);
    }
    if(freeimgdata){
        free(imgdata);
    }
    Device__unref(device);
} 

void Device__load_thumbnail(Device* device, EventQueue * queue){
    EventQueue__insert(queue,_load_thumbnail,device);
}

void profile_changed(GtkComboBox* self, Device * device){
    int new_index = gtk_combo_box_get_active(self);
    if(new_index != device->profile_index){
        device->profile_index = new_index;
        if(device->profile_callback){
            device->profile_callback(device,device->profile_userdata);
        }
    }
}


GtkWidget * Device__create_row (Device * device, char * uri, char* name, char * hardware, char * location){
    GtkWidget *row;
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *image;

    row = gtk_list_box_row_new ();

    grid = gtk_grid_new ();
    g_object_set (grid, "margin", 5, NULL);

    image = gtk_spinner_new ();
    gtk_spinner_start (GTK_SPINNER (image));

    GtkWidget * thumbnail_handle = gtk_event_box_new ();
    device->image_handle = thumbnail_handle;
    gtk_container_add (GTK_CONTAINER (thumbnail_handle), image);
    g_object_set (thumbnail_handle, "margin-end", 10, NULL);
    gtk_grid_attach (GTK_GRID (grid), thumbnail_handle, 0, 1, 1, 3);

    char* markup_name = malloc(strlen(name)+1+7);
    strcpy(markup_name, "<b>");
    strcat(markup_name, name);
    strcat(markup_name, "</b>");
    label = gtk_label_new (NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup_name);
    g_object_set (label, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 2, 1);

    label = gtk_label_new (device->onvif_device->ip);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);

    label = gtk_label_new (hardware);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);

    label = gtk_label_new (location);
    gtk_label_set_ellipsize (GTK_LABEL(label),PANGO_ELLIPSIZE_END);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 3, 1, 1);

    //Create profile dropdown placeholder
    GtkListStore *liststore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    device->profile_dropdown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(liststore));
    g_object_unref(liststore);

    GtkCellRenderer  * column = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(device->profile_dropdown), column, TRUE);

    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(device->profile_dropdown), column,
                                    "cell-background", 0,
                                    "text", 1,
                                    NULL);
                                    
    gtk_widget_set_sensitive (device->profile_dropdown, FALSE);
    gtk_grid_attach (GTK_GRID (grid), device->profile_dropdown, 0, 4, 2, 1);

    g_signal_connect (G_OBJECT (device->profile_dropdown), "changed", G_CALLBACK (profile_changed), device);

    gtk_container_add (GTK_CONTAINER (row), grid);
  
    //For some reason, spinner has a floating ref
    //This is required to keep ability to remove the spinner later
    g_object_ref(image);

    return row;
}

void Device__set_profile_callback(Device * self, void (*profile_callback)(Device *, void *), void * profile_userdata){
    self->profile_callback = profile_callback;
    self->profile_userdata = profile_userdata;
}

gboolean * gui_display_profiles (void * user_data){
    Device * device = (Device *) user_data;
    if(!Device__addref(device) || !device->onvif_device->authorized){
        return FALSE;
    }
    
    GtkListStore *liststore = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(device->profile_dropdown)));
    for (int i = 0; i < device->onvif_device->sizeSrofiles; i++){
        printf("Profile name: %s\n", device->onvif_device->profiles[i].name);
        printf("Profile token: %s\n", device->onvif_device->profiles[i].token);

        gtk_list_store_insert_with_values(liststore, NULL, -1,
                                        // 0, "red",
                                        1, device->onvif_device->profiles[i].name,
                                        -1);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(device->profile_dropdown),0);
    gtk_widget_set_sensitive (device->profile_dropdown, TRUE);

    Device__unref(device);
    return FALSE;
}

//WIP Profile selection
void _load_profiles(void * user_data){
    Device * device = (Device *) user_data;

    if(!Device__addref(device) || !device->onvif_device->authorized){
        return;
    }

    OnvifDevice_get_profiles(device->onvif_device);
    gdk_threads_add_idle((void *)gui_display_profiles,device);

    Device__unref(device);
}

void Device__load_profiles(Device* device, EventQueue * queue){
    EventQueue__insert(queue,_load_profiles,device);
}