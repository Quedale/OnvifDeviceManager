#include "omgr_device_row.h"
#include "onvif_device.h"
#include "clogger.h"
#include "gui_utils.h"
#include "gtkstyledimage.h"

#define OMGR_DEVICE_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

extern char _binary_locked_icon_png_size[];
extern char _binary_locked_icon_png_start[];
extern char _binary_locked_icon_png_end[];

extern char _binary_warning_png_size[];
extern char _binary_warning_png_start[];
extern char _binary_warning_png_end[];

extern char _binary_save_png_size[];
extern char _binary_save_png_start[];
extern char _binary_save_png_end[];

enum {
  PROFILE_CLICKED,
  PROFILE_CHANGED,
  LAST_SIGNAL
};

typedef struct {
    OnvifApp * app;
    OnvifDevice * device;
    OnvifProfile * profile;

    gboolean owned;
    GtkWidget * btn_save;
    GtkWidget * btn_profile;
    GtkWidget * lbl_name;
    GtkWidget * lbl_host;
    GtkWidget * lbl_hardware;
    GtkWidget * lbl_location;
    GtkWidget * image;
    GtkWidget * image_handle;
} OnvifMgrDeviceRowPrivate;

static guint signals[LAST_SIGNAL] = { 0 };

static void OnvifMgrDeviceRow__ownable_interface_init (COwnableObjectInterface *iface);

G_DEFINE_TYPE_WITH_CODE(OnvifMgrDeviceRow, OnvifMgrDeviceRow_, GTK_TYPE_LIST_BOX_ROW, G_ADD_PRIVATE (OnvifMgrDeviceRow)
                                    G_IMPLEMENT_INTERFACE (COWNABLE_TYPE_OBJECT,OnvifMgrDeviceRow__ownable_interface_init))

static void OnvifMgrDeviceRow_change_parent (OnvifMgrDeviceRow * self){
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    if(!GTK_IS_WIDGET(self)){
        priv->owned = FALSE;
        return;
    }
    priv->owned = gtk_widget_get_parent(GTK_WIDGET(self)) != NULL;
}

static gboolean
OnvifMgrDeviceRow_has_owner (COwnableObject *self)
{
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(self));
    return priv->owned;
}

static void
OnvifMgrDeviceRow_disown (COwnableObject *self)
{
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(self));
    priv->owned = TRUE;
}


static void
OnvifMgrDeviceRow__ownable_interface_init (COwnableObjectInterface *iface)
{
  iface->has_owner = OnvifMgrDeviceRow_has_owner;
  iface->disown = OnvifMgrDeviceRow_disown;
}


static GtkWidget * 
OnvifMgrDeviceRow__create_save_btn(OnvifMgrDeviceRow * self){
    GError *error = NULL;
    GtkWidget *button = gtk_button_new();
    GtkWidget * image = GtkStyledImage__new((unsigned char *)_binary_save_png_start, _binary_save_png_end - _binary_save_png_start, 20, 20, error);

    gtk_widget_set_sensitive(button,FALSE);
    gtk_button_set_always_show_image(GTK_BUTTON(button),TRUE);

    if(image){
        gtk_button_set_image(GTK_BUTTON(button), image);
    } else {
        if(error && error->message){
            C_ERROR("Error writing warning png to GtkPixbufLoader : %s",error->message);
        } else {
            C_ERROR("Error writing warning png to GtkPixbufLoader : [null]");
        }
    }

    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);

    GtkCssProvider * cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { padding: 2px; }",-1,NULL); 
    GtkStyleContext * context = gtk_widget_get_style_context(button);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (cssProvider);  

    return button;
}

static void OnvifMgrDeviceRow__btn_profile_clicked (GtkWidget * widget, OnvifMgrDeviceRow * device){
    g_signal_emit (device, signals[PROFILE_CLICKED], 0 /* details */);
}

static GtkWidget * 
OnvifMgrDeviceRow__create_buttons(OnvifMgrDeviceRow * self){
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    priv->btn_save = OnvifMgrDeviceRow__create_save_btn(self);
    g_object_set (priv->btn_save, "margin-end", 3, NULL);
    gtk_widget_set_sensitive (priv->btn_save, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), priv->btn_save,     FALSE, FALSE, 0);

    priv->btn_profile = gtk_button_new_with_label("");
    g_signal_connect (G_OBJECT(priv->btn_profile), "clicked", G_CALLBACK (OnvifMgrDeviceRow__btn_profile_clicked), self);

    GtkCssProvider * cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, "* { min-height: 15px; padding: 0px; padding-bottom: 2px; }",-1,NULL); 
    GtkStyleContext * context = gtk_widget_get_style_context(priv->btn_profile);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (cssProvider);  

    gtk_widget_set_sensitive (priv->btn_profile, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), priv->btn_profile,     TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox,     FALSE, FALSE, 0);
    return vbox;
}

static void
OnvifMgrDeviceRow__create_layout(OnvifMgrDeviceRow * self){
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    GtkWidget * grid = gtk_grid_new();

    priv->lbl_name = gtk_label_new (NULL);
    g_object_set (priv->lbl_name, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (priv->lbl_name, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->lbl_name, 0, 0, 2, 1);

    priv->image = gtk_spinner_new ();
    priv->image_handle = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (priv->image_handle), priv->image);
    gtk_grid_attach (GTK_GRID (grid), priv->image_handle, 0, 1, 1, 3);

    priv->lbl_host = gtk_label_new (NULL);
    g_object_set (priv->lbl_host, "margin-top", 0, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (priv->lbl_host, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->lbl_host, 1, 1, 1, 1);

    priv->lbl_hardware = gtk_label_new (NULL);
    g_object_set (priv->lbl_hardware, "margin-top", 0, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (priv->lbl_hardware, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->lbl_hardware, 1, 2, 1, 1);

    priv->lbl_location = gtk_label_new (NULL);
    gtk_label_set_ellipsize (GTK_LABEL(priv->lbl_location),PANGO_ELLIPSIZE_END);
    g_object_set (priv->lbl_location, "margin-top", 0, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (priv->lbl_location, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->lbl_location, 1, 3, 1, 1);

    GtkWidget * widget = OnvifMgrDeviceRow__create_buttons(self);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 4, 2, 1);

    gtk_container_add (GTK_CONTAINER (self), grid);
    //For some reason, spinner has a floating ref
    //This is required to keep ability to remove the spinner later
    g_object_ref(priv->image);
}

static void
OnvifMgrDeviceRow__realize (GtkWidget *widget)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (widget));

    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(widget));

    //Start loading animation
    gtk_spinner_start (GTK_SPINNER (priv->image));

    if (GTK_WIDGET_CLASS (OnvifMgrDeviceRow__parent_class)->realize)
        (* GTK_WIDGET_CLASS (OnvifMgrDeviceRow__parent_class)->realize) (widget);
}

// void OnvifMgrDeviceRow__ref_notify (gpointer data, GObject* where_the_object_was){
//     C_FATAL("UNREFREENCED\n");
// }

static void
OnvifMgrDeviceRow__init (OnvifMgrDeviceRow * self)
{
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    priv->device = NULL;  
    priv->profile = NULL;
    priv->owned = TRUE;
    
    g_signal_connect (self, "notify::parent", G_CALLBACK (OnvifMgrDeviceRow_change_parent), NULL);

    /* initialize all public and private members to reasonable default values.
    * They are all automatically initialized to 0 to begin with. */
    // g_object_weak_ref(G_OBJECT(self),OnvifMgrDeviceRow__ref_notify,NULL);
    OnvifMgrDeviceRow__create_layout(self);
}

// static void
// OnvifMgrDeviceRow__finalize (GObject * self)
// {
//     g_return_if_fail (self != NULL);
//     g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
//     // OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
//     G_OBJECT_CLASS (OnvifMgrDeviceRow__parent_class)->finalize (self);
// }

static void
OnvifMgrDeviceRow__destroy (GtkWidget *object)
{
    g_return_if_fail (object != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (object));
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(object));
    OnvifDevice__destroy(priv->device);
    OnvifProfile__destroy(priv->profile);

    //GTK may call destroy multiple times. Setting pointer to null to avoid segmentation fault
    priv->device = NULL;
    priv->profile = NULL;

    if (GTK_WIDGET_CLASS (OnvifMgrDeviceRow__parent_class)->destroy)
        (* GTK_WIDGET_CLASS (OnvifMgrDeviceRow__parent_class)->destroy) (object);
}

static void
OnvifMgrDeviceRow__unrealize (GtkWidget *widget)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (widget));

    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(widget));
    if(GTK_IS_SPINNER(priv->image)){
        gtk_spinner_stop (GTK_SPINNER (priv->image));
    }
    
    GTK_WIDGET_CLASS (OnvifMgrDeviceRow__parent_class)->unrealize (widget);
}

static void
OnvifMgrDeviceRow__class_init (OnvifMgrDeviceRowClass * klass)
{
    // GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->realize = OnvifMgrDeviceRow__realize;
    widget_class->unrealize = OnvifMgrDeviceRow__unrealize;
    // object_class->finalize = OnvifMgrDeviceRow__finalize;
    widget_class->destroy = OnvifMgrDeviceRow__destroy;

    signals[PROFILE_CLICKED] =
        g_signal_newv ("profile-clicked",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        0     /* n_params */,
                        NULL  /* param_types */);
    signals[PROFILE_CHANGED] =
        g_signal_newv ("profile-changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        0     /* n_params */,
                        NULL  /* param_types */);

}

GtkWidget* OnvifMgrDeviceRow__new(OnvifApp * app, OnvifDevice * device, char * name, char * hardware, char * location){
    GtkWidget* ret = g_object_new (ONVIFMGR_TYPE_DEVICEROW,
                        NULL);
    OnvifMgrDeviceRow * odevice = ONVIFMGR_DEVICEROW(ret);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (odevice);
    priv->app = app;
    OnvifMgrDeviceRow__set_device(odevice,device);
    OnvifMgrDeviceRow__set_name(odevice,name);
    OnvifMgrDeviceRow__set_hardware(odevice,hardware);
    OnvifMgrDeviceRow__set_location(odevice,location);
    return ret;
}

OnvifApp * OnvifMgrDeviceRow__get_app(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (ONVIFMGR_IS_DEVICEROW (self),NULL);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(self));
    return priv->app;
}

void OnvifMgrDeviceRow__set_device(OnvifMgrDeviceRow * self, OnvifDevice * device){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    OnvifDevice__destroy(priv->device);
    priv->device = device;

    if(device){
        char * host = OnvifDevice__get_host(device);
        gtk_label_set_text(GTK_LABEL(priv->lbl_host),host);
        free(host);
    } else {
        gtk_label_set_text(GTK_LABEL(priv->lbl_host),NULL);
    }
}

OnvifDevice * OnvifMgrDeviceRow__get_device(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (ONVIFMGR_IS_DEVICEROW (self),NULL);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    return priv->device;
}

void OnvifMgrDeviceRow__set_name(OnvifMgrDeviceRow * self, char * name){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    if(name){
        char markup_name[strlen("<b>") + strlen(name) + strlen("</b>") +1]; //= malloc(strlen("<b>") + strlen(name) + strlen("</b>") +1);
        strcpy(markup_name, "<b>");
        strcat(markup_name, name);
        strcat(markup_name, "</b>");
        gtk_label_set_markup(GTK_LABEL(priv->lbl_name),markup_name);
    } else {
        gtk_label_set_text(GTK_LABEL(priv->lbl_name),NULL);
    }
}

void OnvifMgrDeviceRow__set_hardware(OnvifMgrDeviceRow * self, char * hardware){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    gtk_label_set_text(GTK_LABEL(priv->lbl_hardware),hardware);
}

void OnvifMgrDeviceRow__set_location(OnvifMgrDeviceRow * self, char * location){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    gtk_label_set_text(GTK_LABEL(priv->lbl_location),location);
}

gboolean * OnvifMgrDeviceRow__update_profile_btn(void * user_data){
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(user_data));

    if(priv->profile){
        gtk_button_set_label(GTK_BUTTON(priv->btn_profile),OnvifProfile__get_name(priv->profile));
        gtk_widget_set_sensitive (priv->btn_profile, TRUE);
    } else {
        gtk_button_set_label(GTK_BUTTON(priv->btn_profile), NULL);
        gtk_widget_set_sensitive (priv->btn_profile, FALSE);
    }

    g_object_unref(G_OBJECT(user_data));
    return FALSE;
}

void OnvifMgrDeviceRow__set_profile(OnvifMgrDeviceRow * self, OnvifProfile * profile){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    if(OnvifProfile__equals(priv->profile,profile)){
        return;
    }

    priv->profile = OnvifProfile__copy(profile);

    g_object_ref(self);
    gdk_threads_add_idle(G_SOURCE_FUNC(OnvifMgrDeviceRow__update_profile_btn),self);

    g_signal_emit (self, signals[PROFILE_CHANGED], 0 /* details */);
}

OnvifProfile * OnvifMgrDeviceRow__get_profile(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (ONVIFMGR_IS_DEVICEROW (self),NULL);
    C_DEBUG("OnvifMgrDeviceRow__get_profile");
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    return priv->profile;
}

gboolean OnvifMgrDeviceRow__is_selected(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (ONVIFMGR_IS_DEVICEROW (self),FALSE);
    return gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(self));
}

void OnvifMgrDeviceRow__load_thumbnail(OnvifMgrDeviceRow * self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    C_TRACE("OnvifMgrDeviceRow__load_thumbnail");
    GtkWidget *image = NULL;
    GError * error = NULL;
    OnvifSnapshot * snapshot = NULL;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        C_TRAIL("OnvifMgrDeviceRow__load_thumbnail - invalid device");
        return;
    }
    
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    OnvifErrorTypes oerror = OnvifDevice__get_last_error(priv->device);
    if(oerror == ONVIF_ERROR_NONE){
        OnvifMediaService * media_service = OnvifDevice__get_media_service(priv->device);
        snapshot = OnvifMediaService__getSnapshot(media_service,OnvifProfile__get_index(priv->profile));
        if(!snapshot){
            C_ERROR("_priv_Device__load_thumbnail- Error retrieve snapshot.");
            goto warning;
        }
        image = GtkBinaryImage__new((unsigned char *)OnvifSnapshot__get_buffer(snapshot),OnvifSnapshot__get_size(snapshot), -1, 40, error);
    } else if(oerror == ONVIF_ERROR_NOT_AUTHORIZED){
        image = GtkStyledImage__new((unsigned char *)_binary_locked_icon_png_start,_binary_locked_icon_png_end - _binary_locked_icon_png_start, 40, 40, error);
    } else {
        goto warning;
    }

    //Check is device is still valid. (User performed scan before snapshot finished)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        C_TRAIL("OnvifMgrDeviceRow__load_thumbnail - invalid device");
        goto exit;
    }

    //Attempt to get downloaded pixbuf or locked icon
    if(!image){
        if(error && error->message){
            C_ERROR("Error writing png to GtkPixbufLoader : %s",error->message);
        } else {
            C_ERROR("Error writing png to GtkPixbufLoader : [null]");
        }
    }

warning:
    //Check is device is still valid. (User performed scan before snapshot finished)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        C_TRAIL("OnvifMgrDeviceRow__load_thumbnail - invalid device");
        goto exit;
    }

    //Show warning icon in case of failure
    if(!image){
        image = GtkStyledImage__new((unsigned char *)_binary_warning_png_start, _binary_warning_png_end - _binary_warning_png_start, 40, 40, error);
        if(!image){
            if(error && error->message){
                C_ERROR("Error writing warning png to GtkPixbufLoader : %s",error->message);
            } else {
                C_ERROR("Error writing warning png to GtkPixbufLoader : [null]");
            }
            goto exit;
        }
    }
    

    if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        gui_update_widget_image(image,priv->image_handle);
    } else {
        C_TRAIL("OnvifMgrDeviceRow__load_thumbnail - invalid device");
    }

exit:
    OnvifSnapshot__destroy(snapshot);

    C_TRACE("OnvifMgrDeviceRow__load_thumbnail done.");
} 

void OnvifMgrDeviceRow__set_thumbnail(OnvifMgrDeviceRow * self, GtkWidget * image){
    g_return_if_fail (self != NULL);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER (self)) C_TRAIL("OnvifMgrDeviceRow__set_thumbnail - invalid device.");
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    gui_update_widget_image(image,priv->image_handle);
}