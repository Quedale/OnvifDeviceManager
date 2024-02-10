
#include <stddef.h>
#include <stdio.h>
#include "device.h"
#include "gui_utils.h"
#include <netdb.h>
#include <arpa/inet.h>
#include "clogger.h"
#include "dialog/profiles_dialog.h"

extern char _binary_prohibited_icon_png_size[];
extern char _binary_prohibited_icon_png_start[];
extern char _binary_prohibited_icon_png_end[];

extern char _binary_locked_icon_png_size[];
extern char _binary_locked_icon_png_start[];
extern char _binary_locked_icon_png_end[];

extern char _binary_warning_png_size[];
extern char _binary_warning_png_start[];
extern char _binary_warning_png_end[];

extern char _binary_save_png_size[];
extern char _binary_save_png_start[];
extern char _binary_save_png_end[];

struct _Device {
    CObject parent;
    
    OnvifApp * app;
    OnvifDevice * onvif_device;
    GtkWidget * image_handle;
    GtkWidget * profile_btn;

    P_MUTEX_TYPE ref_lock;
    OnvifProfile * selected_profile;
    int selected;

    void (*profile_callback)(Device *, void *);
    void * profile_userdata;
};


void priv_Device__destroy(CObject * self);
void priv_Device__profile_changed(GtkComboBox* self, Device * device);
void _priv_Device__load_thumbnail(void * user_data);

gboolean * gui_Device__display_profile (void * user_data);

void priv_Device__destroy(CObject * self){
    OnvifDevice__destroy(((Device*)self)->onvif_device);
    OnvifProfile__destroy(((Device*)self)->selected_profile);
    P_MUTEX_CLEANUP(self->ref_lock);
}

void _priv_Device__load_thumbnail(void * user_data){
    C_DEBUG("_priv_Device__load_thumbnail");
    GtkWidget *image;
    GError *error = NULL;
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *scaled_pixbuf = NULL;
    double size = -1;
    char * imgdata = NULL;
    OnvifSnapshot * snapshot = NULL;
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();

    Device * device = (Device *) user_data;

    if(!CObject__addref((CObject*)device)){
        C_WARN("_priv_Device__load_thumbnail - invalid device #1");
        return;
    }
    
    OnvifErrorTypes oerror = OnvifDevice__get_last_error(device->onvif_device);
    if(oerror == ONVIF_ERROR_NONE){
        OnvifMediaService * media_service = OnvifDevice__get_media_service(device->onvif_device);
        snapshot = OnvifMediaService__getSnapshot(media_service,OnvifProfile__get_index(Device__get_selected_profile(device)));
        if(!snapshot){
            C_ERROR("_priv_Device__load_thumbnail- Error retrieve snapshot.");
            goto warning;
        }
        imgdata = OnvifSnapshot__get_buffer(snapshot);
        size = OnvifSnapshot__get_size(snapshot);
    } else if(oerror == ONVIF_ERROR_NOT_AUTHORIZED){
        imgdata = _binary_locked_icon_png_start;
        size = _binary_locked_icon_png_end - _binary_locked_icon_png_start;
    } else {
        goto warning;
    }

    //Check is device is still valid. (User performed scan before snapshot finished)
    if(!CObject__is_valid((CObject*)device)){
        C_WARN("_priv_Device__load_thumbnail - invalid device #2");
        goto exit;
    }

    //Attempt to get downloaded pixbuf or locked icon
    if(gdk_pixbuf_loader_write (loader, (unsigned char *)imgdata, size,&error)){
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    } else {
        if(error->message){
            C_ERROR("Error writing png to GtkPixbufLoader : %s",error->message);
        } else {
            C_ERROR("Error writing png to GtkPixbufLoader : [null]");
        }
    }

warning:
    //Check is device is still valid. (User performed scan before snapshot finished)
    if(!CObject__is_valid((CObject*)device)){
        C_WARN("_priv_Device__load_thumbnail - invalid device #3");
        goto exit;
    }

    //Show warning icon in case of failure
    if(!pixbuf){
        if(gdk_pixbuf_loader_write (loader, (unsigned char *)_binary_warning_png_start, _binary_warning_png_end - _binary_warning_png_start,&error)){
            pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        } else {
            if(error->message){
                C_ERROR("Error writing warning png to GtkPixbufLoader : %s",error->message);
            } else {
                C_ERROR("Error writing warning png to GtkPixbufLoader : [null]");
            }
        }
    }

    //Check is device is still valid. (User performed scan before spinner showed)
    if(!CObject__is_valid((CObject*)device)){
        C_WARN("_priv_Device__load_thumbnail - invalid device #4");
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
        if(!CObject__is_valid((CObject*)device)){
            C_WARN("_priv_Device__load_thumbnail - invalid device #5");
            goto exit;
        }

        gui_update_widget_image(image,device->image_handle);
    } else {
        C_ERROR("Failed all thumbnail creation.");
    }

exit:
    if(loader){
        gdk_pixbuf_loader_close(loader,NULL);
        g_object_unref(loader);
    }
    if(scaled_pixbuf){
        g_object_unref(scaled_pixbuf);
    }
    OnvifSnapshot__destroy(snapshot);

    C_TRACE("_priv_Device__load_thumbnail done.");
    CObject__unref((CObject*)device);
} 

gboolean * gui_Device__display_profile (void * user_data){
    Device * device = (Device *) user_data;
    C_DEBUG("gui_Device__display_profile");

    if(!CObject__addref((CObject*)device)){
        C_WARN("gui_Device__display_profile - invalid device");
        return FALSE;
    }

    if(!device->selected_profile){
        gtk_button_set_label(GTK_BUTTON(device->profile_btn),"");
        gtk_widget_set_sensitive (device->profile_btn, FALSE);
    } else {
        gtk_button_set_label(GTK_BUTTON(device->profile_btn),OnvifProfile__get_name(device->selected_profile));
        gtk_widget_set_sensitive (device->profile_btn, TRUE);
    }

    CObject__unref((CObject*)device);
    return FALSE;
}

int Device__set_selected_profile(Device * self, OnvifProfile * profile){
    P_MUTEX_LOCK(self->ref_lock);
    if(OnvifProfile__equals(self->selected_profile,profile)){
        P_MUTEX_UNLOCK(self->ref_lock);
        return 0;
    }
    
    if(self->selected_profile){
        OnvifProfile__destroy(self->selected_profile);
    }
    self->selected_profile = OnvifProfile__copy(profile);
    P_MUTEX_UNLOCK(self->ref_lock);
    gdk_threads_add_idle(G_SOURCE_FUNC(gui_Device__display_profile),self);
    return 1;
}

void Device__init(Device* self, OnvifDevice * onvif_device, OnvifApp * app) {
    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_Device__destroy);
    self->onvif_device = onvif_device;
    self->app = app;
    self->selected=0;
    self->profile_callback = NULL;
    self->profile_userdata = NULL;
    self->selected_profile = NULL;
    P_MUTEX_SETUP(self->ref_lock);
}

Device * Device__create(OnvifApp * app, OnvifDevice * onvif_device){
    Device* result = (Device*) malloc(sizeof(Device));
    Device__init(result, onvif_device, app);
    CObject__set_allocated((CObject *) result);
    return result;
}

OnvifApp * Device__get_app(Device * self){
    return self->app;
}

void Device__load_thumbnail(Device* device, EventQueue * queue){
    C_DEBUG("Device__load_thumbnail");
    EventQueue__insert(queue,_priv_Device__load_thumbnail,device);
}

void Device_popup_profiles (GtkWidget *widget, Device * device) {
    ProfilesDialog * dialog = OnvifApp__get_profiles_dialog(device->app);
    ProfilesDialog__set_device(dialog,device);

    AppDialog__show((AppDialog *) dialog,NULL,NULL,device);
}

GtkWidget * Device__create_save_btn(Device * device){
    GError *error = NULL;
    GtkWidget *image = NULL;
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *scaled_pixbuf = NULL;
    GtkWidget *button = gtk_button_new();

    gtk_widget_set_sensitive(button,FALSE);
    gtk_button_set_always_show_image(GTK_BUTTON(button),TRUE);

    GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
    if(gdk_pixbuf_loader_write (loader, (unsigned char *)_binary_save_png_start, _binary_save_png_end - _binary_save_png_start,&error)){
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    } else {
        if(error->message){
            C_ERROR("Error writing warning png to GtkPixbufLoader : %s",error->message);
        } else {
            C_ERROR("Error writing warning png to GtkPixbufLoader : [null]");
        }
    }

    if(pixbuf){
        int scale = 1;
        scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,15*scale,15*scale,GDK_INTERP_NEAREST);
        image = gtk_image_new_from_pixbuf (scaled_pixbuf);
    }

    if(image){
        gtk_button_set_image(GTK_BUTTON(button), image);
    } else {
        C_ERROR("Save icon not created.");
    }

    gdk_pixbuf_loader_close(loader,NULL);
    g_object_unref(loader);
    if(scaled_pixbuf){
        g_object_unref(scaled_pixbuf);
    }

    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);

    GtkCssProvider * cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { padding: 2px; }",-1,NULL); 
    GtkStyleContext * context = gtk_widget_get_style_context(button);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (cssProvider);  

    // gtk_button_set_relief(GTK_BUTTON(button),GTK_RELIEF_NONE);

    return button;
}

GtkWidget * Device__create_row_profile(Device * device){
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget * save_btn = Device__create_save_btn(device);
    g_object_set (save_btn, "margin-end", 3, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), save_btn,     FALSE, FALSE, 0);

    device->profile_btn = gtk_button_new_with_label("");

    GtkCssProvider * cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { min-height: 15px; padding: 0px; padding-bottom: 2px; }",-1,NULL); 
    GtkStyleContext * context = gtk_widget_get_style_context(device->profile_btn);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (cssProvider);  

    gtk_widget_set_sensitive (device->profile_btn, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), device->profile_btn,     TRUE, TRUE, 0);

    g_signal_connect (device->profile_btn, "clicked", G_CALLBACK (Device_popup_profiles), device);

    gtk_box_pack_start(GTK_BOX(vbox), hbox,     FALSE, FALSE, 0);
    return vbox;
}


GtkWidget * Device__create_row (Device * device, char * uri, char* name, char * hardware, char * location){
    GtkWidget *row;
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *image;

    row = gtk_list_box_row_new ();

    grid = gtk_grid_new ();
    // g_object_set (grid, "margin", 5, NULL);

    image = gtk_spinner_new ();
    gtk_spinner_start (GTK_SPINNER (image));

    GtkWidget * thumbnail_handle = gtk_event_box_new ();
    device->image_handle = thumbnail_handle;
    gtk_container_add (GTK_CONTAINER (thumbnail_handle), image);
    gtk_grid_attach (GTK_GRID (grid), thumbnail_handle, 0, 1, 1, 3);

    if(!name){
        name = "N/A";
    }
    char markup_name[strlen("<b>") + strlen(name) + strlen("</b>") +1]; //= malloc(strlen("<b>") + strlen(name) + strlen("</b>") +1);
    strcpy(markup_name, "<b>");
    strcat(markup_name, name);
    strcat(markup_name, "</b>");
    label = gtk_label_new (NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup_name);
    g_object_set (label, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 2, 1);

    char * dev_host = OnvifDevice__get_host(device->onvif_device);
    label = gtk_label_new (dev_host);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);
    free(dev_host);

    label = gtk_label_new (hardware);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);

    label = gtk_label_new (location);
    gtk_label_set_ellipsize (GTK_LABEL(label),PANGO_ELLIPSIZE_END);
    g_object_set (label, "margin-top", 5, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_grid_attach (GTK_GRID (grid), label, 1, 3, 1, 1);

    GtkWidget * widget = Device__create_row_profile(device);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 4, 2, 1);

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

void Device__select_default_profile(Device* device){
    C_DEBUG("Device__select_default_profile");
    if(device->selected_profile){
        C_DEBUG("Device__select_default_profile - profile already selected\n");
        return;
    }
    if(OnvifDevice__get_last_error(device->onvif_device) != ONVIF_ERROR_NONE){
        C_WARN("_priv_Device__load_profiles - Unable to load profiles");
        return;
    }

    OnvifMediaService * media_service = OnvifDevice__get_media_service(device->onvif_device);
    OnvifProfiles * profiles = OnvifMediaService__get_profiles(media_service);
    if(CObject__is_valid((CObject*)device) && OnvifDevice__get_last_error(device->onvif_device) == ONVIF_ERROR_NONE){
        if(profiles && OnvifProfiles__get_size(profiles) > 0){
            //Select index 0 by default TODO Remember previous selection?
            Device__set_selected_profile(device, OnvifProfiles__get_profile(profiles,0));
        }
    }
    OnvifProfiles__destroy(profiles);
}

OnvifDevice * Device__get_device(Device * self){
    return self->onvif_device;
}

int Device__is_selected(Device * self){
    return self->selected;
}

void Device__set_selected(Device * self, int selected){
    self->selected = selected;
}

OnvifProfile * Device__get_selected_profile(Device * self){
    OnvifProfile * ret = NULL;
    P_MUTEX_LOCK(self->ref_lock);
    ret = self->selected_profile;
    P_MUTEX_UNLOCK(self->ref_lock);
    return ret;
}

void Device__set_thumbnail(Device * self, GtkWidget * image){
    gui_update_widget_image(image,self->image_handle);
}