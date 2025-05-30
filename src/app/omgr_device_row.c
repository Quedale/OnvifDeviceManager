#include "omgr_device_row.h"
#include "onvif_device.h"
#include "clogger.h"
#include "gui_utils.h"
#include "gtkstyledimage.h"
#include "../utils/omgr_serializable_interface.h"

#define OMGR_DEVICE_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

#define OMGR_DEVICE_URL_PREFIX "URL:"
#define OMGR_DEVICE_USER_PREFIX "USER:"
#define OMGR_DEVICE_PASS_PREFIX "PASS:"
#define OMGR_DEVICE_NAME_PREFIX "NAME:"
#define OMGR_DEVICE_HW_PREFIX "HW:"
#define OMGR_DEVICE_LOC_PREFIX "LOC:"

extern char _binary_locked_icon_png_size[];
extern char _binary_locked_icon_png_start[];
extern char _binary_locked_icon_png_end[];

extern char _binary_warning_png_size[];
extern char _binary_warning_png_start[];
extern char _binary_warning_png_end[];

extern char _binary_trash_png_size[];
extern char _binary_trash_png_start[];
extern char _binary_trash_png_end[];

enum
{
    PROP_APP = 1,
    PROP_DEVICE = 2,
    PROP_NAME = 3,
    PROP_HARDWARE = 4,
    PROP_LOCATION = 5,
    N_PROPERTIES
};

enum {
    PROFILE_CLICKED,
    PROFILE_CHANGED,
    LAST_SIGNAL
};

typedef struct {
    OnvifApp * app;
    OnvifDevice * device;
    OnvifMediaProfile * profile;

    gboolean owned;
    gboolean init;
    GtkWidget * button_grid;
    GtkWidget * btn_trash;
    GtkWidget * btn_profile;
    GtkWidget * lbl_name;
    GtkWidget * lbl_host;
    GtkWidget * lbl_hardware;
    GtkWidget * lbl_location;
    GtkWidget * image;
    GtkWidget * image_handle;
} OnvifMgrDeviceRowPrivate;

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void OnvifMgrDeviceRow__ownable_interface_init (COwnableObjectInterface *iface);
static void OnvifMgrDeviceRow__serializable_interface_init (OnvifMgrSerializableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(OnvifMgrDeviceRow, OnvifMgrDeviceRow_, GTK_TYPE_LIST_BOX_ROW, 
                                    G_ADD_PRIVATE (OnvifMgrDeviceRow)
                                    G_IMPLEMENT_INTERFACE (COWNABLE_TYPE_OBJECT,OnvifMgrDeviceRow__ownable_interface_init)
                                    G_IMPLEMENT_INTERFACE (OMGR_TYPE_SERIALIZABLE,OnvifMgrDeviceRow__serializable_interface_init))

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
    priv->owned = FALSE;
    gtk_widget_unparent(GTK_WIDGET(self));
    g_object_unref(self);
}


static void
OnvifMgrDeviceRow__ownable_interface_init (COwnableObjectInterface *iface)
{
  iface->has_owner = OnvifMgrDeviceRow_has_owner;
  iface->disown = OnvifMgrDeviceRow_disown;
}

static unsigned char * 
OnvifMgrDeviceRow__serialize (OnvifMgrSerializable  *self, int * serialized_length){
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(self));
    OnvifDeviceService * device_service = OnvifDevice__get_device_service(priv->device);
    char * url = OnvifBaseService__get_endpoint((OnvifBaseService*)device_service);
    OnvifCredentials * credentials = OnvifDevice__get_credentials(priv->device);

    unsigned char * output = NULL;
    if(credentials){
        char * user = OnvifCredentials__get_username(credentials);
        char * pass = OnvifCredentials__get_password(credentials);
        const char * location = gtk_label_get_text(GTK_LABEL(priv->lbl_location));
        const char * hardware = gtk_label_get_text(GTK_LABEL(priv->lbl_hardware));
        const char * name = gtk_label_get_text(GTK_LABEL(priv->lbl_name));

        int len = strlen(OMGR_DEVICE_URL_PREFIX);
        len += strlen(url) + 1;
        if(user && strlen(user)) {
            len += strlen(OMGR_DEVICE_USER_PREFIX);
            len += strlen(user) +1;
        }
        if(pass && strlen(pass)) {
            len += strlen(OMGR_DEVICE_PASS_PREFIX);
            len += strlen(pass) +1;
        }

        len += strlen(OMGR_DEVICE_NAME_PREFIX);
        len += strlen(name) + 1;
        len += strlen(OMGR_DEVICE_HW_PREFIX);
        len += strlen(hardware) + 1;
        len += strlen(OMGR_DEVICE_LOC_PREFIX);
        len += strlen(location) + 1;

        output = malloc(len);
        *serialized_length = 0;

        memcpy(output,OMGR_DEVICE_URL_PREFIX,strlen(OMGR_DEVICE_URL_PREFIX));
        *serialized_length += strlen(OMGR_DEVICE_URL_PREFIX);
        memcpy(&output[*serialized_length],url,strlen(url) + 1);
        *serialized_length += strlen(url)+1;

        memcpy(&output[*serialized_length],OMGR_DEVICE_NAME_PREFIX,strlen(OMGR_DEVICE_NAME_PREFIX));
        *serialized_length += strlen(OMGR_DEVICE_NAME_PREFIX);
        memcpy(&output[*serialized_length],name,strlen(name)+1);
        *serialized_length += strlen(name)+1;

        memcpy(&output[*serialized_length],OMGR_DEVICE_HW_PREFIX,strlen(OMGR_DEVICE_HW_PREFIX));
        *serialized_length += strlen(OMGR_DEVICE_HW_PREFIX);
        memcpy(&output[*serialized_length],hardware,strlen(hardware)+1);
        *serialized_length += strlen(hardware)+1;

        memcpy(&output[*serialized_length],OMGR_DEVICE_LOC_PREFIX,strlen(OMGR_DEVICE_LOC_PREFIX));
        *serialized_length += strlen(OMGR_DEVICE_LOC_PREFIX);
        memcpy(&output[*serialized_length],location,strlen(location)+1);
        *serialized_length += strlen(location)+1;

        if(user){
            memcpy(&output[*serialized_length],OMGR_DEVICE_USER_PREFIX,strlen(OMGR_DEVICE_USER_PREFIX));
            *serialized_length += strlen(OMGR_DEVICE_USER_PREFIX);
            memcpy(&output[*serialized_length],user,strlen(user)+1);
            *serialized_length += strlen(user)+1;
            free(user);
        }
        if(pass){
            memcpy(&output[*serialized_length],OMGR_DEVICE_PASS_PREFIX,strlen(OMGR_DEVICE_PASS_PREFIX));
            *serialized_length += strlen(OMGR_DEVICE_PASS_PREFIX);
            memcpy(&output[*serialized_length],pass,strlen(pass)+1);
            *serialized_length += strlen(pass)+1;
            free(pass);
        }

    } else {
        output = malloc(strlen(url)+1);
        *serialized_length = strlen(url)+1;
        memcpy(output,url,*serialized_length);
    }

    free(url);
    return output;
}

static OnvifMgrSerializable * 
OnvifMgrDeviceRow__unserialize (unsigned char * data, int length){
    int data_read = 0;
    char * url = NULL;
    char * user = NULL;
    char * pass = NULL;
    char * name = NULL;
    char * location = NULL;
    char * hardware = NULL;

    while(data_read < length){
        int line_len = strlen((char*)&data[data_read])+1;

        if(strncmp((char *)&data[data_read], OMGR_DEVICE_URL_PREFIX, strlen(OMGR_DEVICE_URL_PREFIX)) == 0){
            url = (char*) &data[data_read + strlen(OMGR_DEVICE_URL_PREFIX)];
        } else if(strncmp((char *)&data[data_read], OMGR_DEVICE_USER_PREFIX, strlen(OMGR_DEVICE_USER_PREFIX)) == 0){
            user = (char*) &data[data_read + strlen(OMGR_DEVICE_USER_PREFIX)];
        } else if(strncmp((char *)&data[data_read], OMGR_DEVICE_PASS_PREFIX, strlen(OMGR_DEVICE_PASS_PREFIX)) == 0){
            pass = (char*) &data[data_read + strlen(OMGR_DEVICE_PASS_PREFIX)];
        } else if(strncmp((char *)&data[data_read], OMGR_DEVICE_NAME_PREFIX, strlen(OMGR_DEVICE_NAME_PREFIX)) == 0){
            name = (char*) &data[data_read + strlen(OMGR_DEVICE_NAME_PREFIX)];
        } else if(strncmp((char *)&data[data_read], OMGR_DEVICE_HW_PREFIX, strlen(OMGR_DEVICE_HW_PREFIX)) == 0){
            hardware = (char*) &data[data_read + strlen(OMGR_DEVICE_HW_PREFIX)];
        } else if(strncmp((char *)&data[data_read], OMGR_DEVICE_LOC_PREFIX, strlen(OMGR_DEVICE_LOC_PREFIX)) == 0){
            location = (char*) &data[data_read + strlen(OMGR_DEVICE_LOC_PREFIX)];
        } 
        data_read += line_len;
    }

    g_return_val_if_fail(url != NULL,NULL);

    OnvifDevice * onvif_dev = OnvifDevice__new(url);
    if(!OnvifDevice__is_valid(onvif_dev)){
        C_ERROR("Invalid URL provided\n");
        g_object_unref(onvif_dev);
        return NULL;
    }
    OnvifCredentials * credentials = OnvifDevice__get_credentials(onvif_dev);
    if(user) OnvifCredentials__set_username(credentials,user);
    if(pass) OnvifCredentials__set_password(credentials,pass);

    return OMGR_SERIALIZABLE(OnvifMgrDeviceRow__new (NULL, onvif_dev, name, hardware, location));
}

static void
OnvifMgrDeviceRow__serializable_interface_init (OnvifMgrSerializableInterface *iface)
{
    iface->serialize = OnvifMgrDeviceRow__serialize;
    iface->unserialize = OnvifMgrDeviceRow__unserialize;
}

static void 
OnvifMgrDeviceRow__trash (GtkWidget * widget, OnvifMgrDeviceRow * device){
    gtk_widget_destroy(GTK_WIDGET(device));
}

static GtkWidget * 
OnvifMgrDeviceRow__create_trash_btn(OnvifMgrDeviceRow * self){
    GError *error = NULL;
    GtkWidget *button = gtk_button_new();
    GtkWidget * image = GtkStyledImage__new((unsigned char *)_binary_trash_png_start, _binary_trash_png_end - _binary_trash_png_start, 20, 20, error);

    g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK (OnvifMgrDeviceRow__trash), self);
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

    safely_set_widget_css(button,"* { padding: 2px; }");
    return button;
}

static void OnvifMgrDeviceRow__btn_profile_clicked (GtkWidget * widget, OnvifMgrDeviceRow * device){
    g_signal_emit (device, signals[PROFILE_CLICKED], 0 /* details */);
}

static gboolean 
OnvifMgrDeviceRow__attach_buttons(void * user_data){
    OnvifMgrDeviceRow * self = ONVIFMGR_DEVICEROW(user_data);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    //Creating the trash isn't safe casuing gtk_css_value_inherit_free error
    priv->btn_trash = OnvifMgrDeviceRow__create_trash_btn(self);
    g_object_set (priv->btn_trash, "margin-end", 3, NULL);

    gtk_box_pack_start(GTK_BOX(hbox), priv->btn_trash,     FALSE, FALSE, 0);

    priv->btn_profile = gtk_button_new_with_label("");
    g_signal_connect (G_OBJECT(priv->btn_profile), "clicked", G_CALLBACK (OnvifMgrDeviceRow__btn_profile_clicked), self);

    safely_set_widget_css(priv->btn_profile,"* { min-height: 15px; padding: 0px; padding-bottom: 2px; }");
    
    gtk_widget_set_sensitive (priv->btn_profile, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), priv->btn_profile,     TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox,     FALSE, FALSE, 0);

    gtk_grid_attach (GTK_GRID (priv->button_grid), vbox, 0, 4, 2, 1);

    gtk_widget_show_all(priv->button_grid); //Call show in case it gets added after its parent is already visible
    return FALSE;
}

static void
OnvifMgrDeviceRow__create_layout(OnvifMgrDeviceRow * self){
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    priv->button_grid = gtk_grid_new();

    priv->lbl_name = gtk_label_new (NULL);
    PangoAttrList *const Attrs = pango_attr_list_new();
    PangoAttribute *const boldAttr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
    pango_attr_list_insert(Attrs, boldAttr);
    gtk_label_set_attributes(GTK_LABEL(priv->lbl_name),Attrs);
    pango_attr_list_unref(Attrs);

    g_object_set (priv->lbl_name, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (priv->lbl_name, TRUE);
    gtk_grid_attach (GTK_GRID (priv->button_grid), priv->lbl_name, 0, 0, 2, 1);

    priv->image = gtk_spinner_new ();
    priv->image_handle = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (priv->image_handle), priv->image);
    gtk_grid_attach (GTK_GRID (priv->button_grid), priv->image_handle, 0, 1, 1, 3);

    priv->lbl_host = gtk_label_new (NULL);
    g_object_set (priv->lbl_host, "margin-top", 0, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (priv->lbl_host, TRUE);
    gtk_grid_attach (GTK_GRID (priv->button_grid), priv->lbl_host, 1, 1, 1, 1);

    priv->lbl_hardware = gtk_label_new (NULL);
    g_object_set (priv->lbl_hardware, "margin-top", 0, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (priv->lbl_hardware, TRUE);
    gtk_grid_attach (GTK_GRID (priv->button_grid), priv->lbl_hardware, 1, 2, 1, 1);

    priv->lbl_location = gtk_label_new (NULL);
    gtk_label_set_ellipsize (GTK_LABEL(priv->lbl_location),PANGO_ELLIPSIZE_END);
    g_object_set (priv->lbl_location, "margin-top", 0, "margin-end", 5, NULL);
    gtk_widget_set_hexpand (priv->lbl_location, TRUE);
    gtk_grid_attach (GTK_GRID (priv->button_grid), priv->lbl_location, 1, 3, 1, 1);

    //Dispatch image creation using GUI thread, because GtkImage construction isn't safe
    g_idle_add(OnvifMgrDeviceRow__attach_buttons,self);

    gtk_container_add (GTK_CONTAINER (self), priv->button_grid);
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
    priv->app = NULL;
    priv->device = NULL;  
    priv->profile = NULL;
    priv->owned = TRUE;
    priv->init = FALSE;

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

    //GTK may call destroy multiple times. Setting pointer to null to avoid segmentation fault
    if(priv->device){
        g_object_unref(priv->device);
        priv->device = NULL;
    }

    if(priv->profile){
        g_object_unref(priv->profile);
        priv->profile = NULL;
    }

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
OnvifMgrDeviceRow__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifMgrDeviceRow * self = ONVIFMGR_DEVICEROW (object);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    switch (prop_id){
        case PROP_APP:
            priv->app = ONVIFMGR_APP(g_value_get_object (value));
            break;
        case PROP_DEVICE:
            if(priv->device) g_object_unref(priv->device);
            priv->device = g_value_get_pointer (value);
            if(priv->device){
                char * host = OnvifDevice__get_host(priv->device);
                gui_set_label_text(priv->lbl_host,host);
                free(host);
            } else {
                gui_set_label_text(priv->lbl_host,NULL);
            }
            break;
        case PROP_NAME:
            gui_set_label_text(priv->lbl_name,(char*)g_value_get_string(value));
            break;
        case PROP_HARDWARE:
            gui_set_label_text(priv->lbl_hardware,(char*)g_value_get_string(value));
            return;
        case PROP_LOCATION:
            gui_set_label_text(priv->lbl_location,(char*)g_value_get_string(value));
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrDeviceRow__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifMgrDeviceRow *thread = ONVIFMGR_DEVICEROW (object);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (thread);
    switch (prop_id){
        case PROP_APP:
            g_value_set_object (value, priv->app);
            break;
        case PROP_DEVICE:
            g_value_set_pointer (value, priv->device);
            break;
        case PROP_NAME:
            g_value_set_string(value,gtk_label_get_text(GTK_LABEL(priv->lbl_name)));
            break;
        case PROP_HARDWARE:
            g_value_set_string(value,gtk_label_get_text(GTK_LABEL(priv->lbl_hardware)));
            return;
        case PROP_LOCATION:
            g_value_set_string(value,gtk_label_get_text(GTK_LABEL(priv->lbl_location)));
            return;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrDeviceRow__class_init (OnvifMgrDeviceRowClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifMgrDeviceRow__set_property;
    object_class->get_property = OnvifMgrDeviceRow__get_property;
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

    obj_properties[PROP_APP] =
        g_param_spec_object ("app",
                            "OnvifApp",
                            "Pointer to OnvifApp parent.",
                            ONVIFMGR_TYPE_APP  /* default value */,
                            G_PARAM_READWRITE);

    obj_properties[PROP_DEVICE] =
        g_param_spec_pointer ("device",
                            "OnvifDevice",
                            "Pointer to OnvifDevice parent.",
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    obj_properties[PROP_NAME] =
        g_param_spec_string ("name",
                            "OnvifName",
                            "Onvif name from scopes",
                            NULL,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_LOCATION] =
        g_param_spec_string ("location",
                            "OnvifLocation",
                            "Onvif location from scopes",
                            NULL,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_HARDWARE] =
        g_param_spec_string ("hardware",
                            "OnvifHardware",
                            "Onvif hardware from scopes",
                            NULL,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

GtkWidget* OnvifMgrDeviceRow__new(OnvifApp * app, OnvifDevice * device, char * name, char * hardware, char * location){
    return g_object_new (ONVIFMGR_TYPE_DEVICEROW,
                        "app",app,
                        "device",device,
                        "name",name,
                        "hardware",hardware,
                        "location",location,
                        NULL);
}

OnvifApp * OnvifMgrDeviceRow__get_app(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (ONVIFMGR_IS_DEVICEROW (self),NULL);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (ONVIFMGR_DEVICEROW(self));
    return priv->app;
}

OnvifDevice * OnvifMgrDeviceRow__get_device(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (ONVIFMGR_IS_DEVICEROW (self),NULL);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    return priv->device;
}

gboolean OnvifMgrDeviceRow__update_profile_btn(void * user_data){
    OnvifMgrDeviceRow * self = ONVIFMGR_DEVICEROW(user_data);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    
    if(priv->profile){
        gtk_button_set_label(GTK_BUTTON(priv->btn_profile),OnvifMediaProfile__get_name(priv->profile));
        gtk_widget_set_sensitive (priv->btn_profile, TRUE);
    } else {
        gtk_button_set_label(GTK_BUTTON(priv->btn_profile), NULL);
        gtk_widget_set_sensitive (priv->btn_profile, FALSE);
    }

    g_object_unref(G_OBJECT(user_data));
    return FALSE;
}

void OnvifMgrDeviceRow__set_profile(OnvifMgrDeviceRow * self, OnvifMediaProfile * profile){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    if(!profile && priv->profile){
        g_object_unref(priv->profile);
        priv->profile = NULL;
        return;
    }

    if(priv->profile == profile){
        return;
    }

    priv->profile = profile;
    g_object_ref(priv->profile);
    g_signal_emit (self, signals[PROFILE_CHANGED], 0);

    g_object_ref(self);
    gdk_threads_add_idle(G_SOURCE_FUNC(OnvifMgrDeviceRow__update_profile_btn),self);
}

OnvifMediaProfile * OnvifMgrDeviceRow__get_profile(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (ONVIFMGR_IS_DEVICEROW (self),NULL);
    ONVIFMGR_DEVICEROW_DEBUG("%s OnvifMgrDeviceRow__get_profile",self);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    return priv->profile;
}

gboolean OnvifMgrDeviceRow__is_selected(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (ONVIFMGR_IS_DEVICEROW (self),FALSE);
    return gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(self));
}

static gboolean
OnvifMgrDeviceRow__display_snapshot(void * user_data){
    void ** arr = (void**)user_data;
    OnvifMgrDeviceRow * self = ONVIFMGR_DEVICEROW(arr[0]);
    OnvifMediaSnapshot * snapshot = arr[1];
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    GError * error = NULL;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        C_TRAIL("OnvifMgrDeviceRow__display_snapshot - invalid device");
        goto exit;
    }

    GtkWidget *image = NULL;
    SoapFault * fault = SoapObject__get_fault(SOAP_OBJECT(snapshot));
    switch(*fault){
        case SOAP_FAULT_NONE:
            image = GtkBinaryImage__new((unsigned char *)OnvifMediaSnapshot__get_buffer(snapshot),OnvifMediaSnapshot__get_size(snapshot), -1, 40, error);
            break;
        case SOAP_FAULT_ACTION_NOT_SUPPORTED:
            goto unsupported;
        case SOAP_FAULT_CONNECTION_ERROR:
        case SOAP_FAULT_NOT_VALID:
        case SOAP_FAULT_UNAUTHORIZED:
        case SOAP_FAULT_UNEXPECTED:
        default:
            goto warning;
    }

    //Attempt to get downloaded pixbuf or locked icon
    if(!image){
        if(error){
            if(error->message){
                C_ERROR("Error writing png to GtkPixbufLoader : [%d] %s", error->code, error->message);
            } else {
                C_ERROR("Error writing png to GtkPixbufLoader : [%d]", error->code, error->message);
            }
        } else {
            C_ERROR("Error writing png to GtkPixbufLoader : [null]");
        }
    }

warning:
    //Check is device is still valid. (User performed scan before snapshot finished)
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        C_TRAIL("OnvifMgrDeviceRow__display_snapshot - invalid device");
        goto exit;
    }

    //Show warning icon in case of failure
    if(!image){
        image = GtkStyledImage__new((unsigned char *)_binary_warning_png_start, _binary_warning_png_end - _binary_warning_png_start, 40, 40, error);
        if(!image){
            if(error){
                if(error->message){
                    C_ERROR("Error writing warning png to GtkPixbufLoader : [%d] %s", error->code, error->message);
                } else {
                    C_ERROR("Error writing warning png to GtkPixbufLoader : [%d]", error->code, error->message);
                }
            } else {
                C_ERROR("Error writing warning png to GtkPixbufLoader : [null]");
            }
            goto exit;
        }
    }
    

    if(ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        gtk_container_foreach (GTK_CONTAINER (priv->image_handle), (GtkCallback)gui_widget_destroy, NULL);
        gtk_container_add (GTK_CONTAINER (priv->image_handle), image);
        gtk_widget_show (image);
    } else {
        ONVIFMGR_DEVICEROW_TRAIL("%s OnvifMgrDeviceRow__display_snapshot invalid device",self);
    }
    goto exit;

unsupported:
    //Snapshot unsupported by camera. No longer need image handle
    if(priv->image_handle){
        gtk_widget_destroy(priv->image_handle);
        priv->image_handle = NULL;
    }
exit:
    free(user_data);
    if(snapshot)
        g_object_unref(snapshot);

    ONVIFMGR_DEVICEROW_TRACE("%s OnvifMgrDeviceRow__display_snapshot done",self);
    return FALSE;
}

static gboolean
OnvifMgrDeviceRow__display_locked(void * user_data){
    OnvifMgrDeviceRow * self = ONVIFMGR_DEVICEROW(user_data);
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    GError * error = NULL;

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        C_TRAIL("OnvifMgrDeviceRow__display_locked - invalid device");
        return FALSE;
    }
    
    GtkWidget *image = GtkStyledImage__new((unsigned char *)_binary_locked_icon_png_start,_binary_locked_icon_png_end - _binary_locked_icon_png_start, 40, 40, error);
    gtk_container_foreach (GTK_CONTAINER (priv->image_handle), (GtkCallback)gui_widget_destroy, NULL);
    gtk_container_add (GTK_CONTAINER (priv->image_handle), image);
    gtk_widget_show (image);

    ONVIFMGR_DEVICEROW_TRACE("%s OnvifMgrDeviceRow__display_locked",self);
    return FALSE;
}

void OnvifMgrDeviceRow__load_thumbnail(OnvifMgrDeviceRow * self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    ONVIFMGR_DEVICEROW_TRACE("%s OnvifMgrDeviceRow__load_thumbnail",self);

    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self) || QueueEvent__is_cancelled(QueueEvent__get_current())){
        C_TRAIL("OnvifMgrDeviceRow__load_thumbnail - invalid device");
        return;
    }
    
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);

    if(OnvifDevice__is_authenticated(priv->device)){
        OnvifMediaService * media_service = OnvifDevice__get_media_service(priv->device);
        OnvifMediaSnapshot * snapshot = OnvifMediaService__getSnapshot(media_service,(priv->profile) ? OnvifMediaProfile__get_index(priv->profile) : 0);
        if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self) || QueueEvent__is_cancelled(QueueEvent__get_current())){
            C_TRAIL("OnvifMgrDeviceRow__load_thumbnail - invalid device");
            return;
        }
        void ** data = malloc(sizeof(void*)*2);
        data[0] = self;
        data[1] = snapshot;
        g_idle_add(OnvifMgrDeviceRow__display_snapshot,data);
    } else {
        g_idle_add(OnvifMgrDeviceRow__display_locked,self);
    }
    ONVIFMGR_DEVICEROW_TRACE("%s OnvifMgrDeviceRow__load_thumbnail done",self);
} 

static gboolean 
OnvifMgrEncryptedStore__gui_set_scopes(void * user_data){
    void ** arr = (void*)user_data;
    OnvifMgrDeviceRow * self = ONVIFMGR_DEVICEROW(arr[0]);
    OnvifScopes * scopes = (OnvifScopes *) arr[1];
    char * name = OnvifScopes__extract_scope(scopes,"name");
    char * hardware = OnvifScopes__extract_scope(scopes,"hardware");
    char * location = OnvifScopes__extract_scope(scopes,"location");

    g_object_set(self, 
            "name", name,
            "hardware",hardware,
            "location", location,
            NULL);

    free(name);
    free(hardware);
    free(location);
    free(user_data);
    g_object_unref(scopes);
    return FALSE;
}

void OnvifMgrDeviceRow__load_scopedata(OnvifMgrDeviceRow * self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_DEVICEROW (self));
    ONVIFMGR_DEVICEROW_TRACE("%s OnvifMgrDeviceRow__load_scopedata",self);
    
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER(self)){
        C_TRAIL("OnvifMgrDeviceRow__load_scopedata - invalid device");
        return;
    }

    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    if(OnvifDevice__is_authenticated(priv->device)){
        OnvifDeviceService * devserv = OnvifDevice__get_device_service(priv->device);
        OnvifScopes * scopes = OnvifDeviceService__getScopes(devserv);
        SoapFault fault = *SoapObject__get_fault(SOAP_OBJECT(scopes));
        void ** input;
        switch(fault){
            case SOAP_FAULT_NONE:
                input = malloc(sizeof(void*) *2);
                input[0] = self;
                input[1] = scopes;
                g_idle_add(G_SOURCE_FUNC(OnvifMgrEncryptedStore__gui_set_scopes),input);
                break;
            case SOAP_FAULT_ACTION_NOT_SUPPORTED:
            case SOAP_FAULT_CONNECTION_ERROR:
            case SOAP_FAULT_NOT_VALID:
            case SOAP_FAULT_UNAUTHORIZED:
            case SOAP_FAULT_UNEXPECTED:
            default:
                //TODO Display error labels
                g_object_unref(scopes);
                break;
        }
    }
}

void OnvifMgrDeviceRow__set_thumbnail(OnvifMgrDeviceRow * self, GtkWidget * image){
    g_return_if_fail (self != NULL);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER (self)) C_TRAIL("OnvifMgrDeviceRow__set_thumbnail - invalid device.");
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    gui_update_widget_image(image,priv->image_handle);
}

void OnvifMgrDeviceRow__set_initialized(OnvifMgrDeviceRow * self){
    g_return_if_fail (self != NULL);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER (self)) C_TRAIL("OnvifMgrDeviceRow__set_initialized - invalid device.");
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    priv->init = TRUE;
}

gboolean OnvifMgrDeviceRow__is_initialized(OnvifMgrDeviceRow * self){
    g_return_val_if_fail (self != NULL, FALSE);
    if(!ONVIFMGR_DEVICEROWROW_HAS_OWNER (self)) C_TRAIL("OnvifMgrDeviceRow__is_initialized - invalid device.");
    OnvifMgrDeviceRowPrivate *priv = OnvifMgrDeviceRow__get_instance_private (self);
    return priv->init;
}