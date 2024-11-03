#include "omgr_msg_dialog.h"
#include "clogger.h"

#define MSG_DIALOG_SUBMIT_LABEL "Ok"

enum
{
  PROP_ICON = 1,
  PROP_MSG = 2,
  N_PROPERTIES
};

typedef struct {
    GtkWidget * lblmessage;
    GtkWidget * lbltitle;
    GtkWidget * image_handle;
} OnvifMgrMsgDialogPrivate;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (OnvifMgrMsgDialog, OnvifMgrMsgDialog_, ONVIFMGR_TYPE_APPDIALOG)

static GtkWidget * 
OnvifMgrMsgDialog__create_ui(OnvifMgrAppDialog * app_dialog){
    OnvifMgrMsgDialogPrivate *priv = OnvifMgrMsgDialog__get_instance_private (ONVIFMGR_MSGDIALOG(app_dialog));
    GtkWidget * grid = gtk_grid_new ();

    priv->lblmessage = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(priv->lblmessage),1);
    gtk_widget_set_hexpand (priv->lblmessage, TRUE);
    gtk_widget_set_vexpand (priv->lblmessage, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->lblmessage, 0, 1, 3, 1);
    gtk_label_set_justify(GTK_LABEL(priv->lblmessage), GTK_JUSTIFY_CENTER);

    GtkWidget * empty = gtk_label_new("");
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (grid), empty, 0, 0, 1, 1);

    priv->image_handle = gtk_event_box_new ();
    g_object_set (priv->image_handle, "margin-top", 20, NULL);
    gtk_grid_attach (GTK_GRID (grid), priv->image_handle, 1, 0, 1, 1);

    empty = gtk_label_new("");
    gtk_widget_set_hexpand (empty, TRUE);
    gtk_grid_attach (GTK_GRID (grid), empty, 2, 0, 1, 1);

    return grid;
}

static void 
OnvifMgrMsgDialog__container_remove_swapped(GtkWidget * widget, gpointer user_data){
    gtk_container_remove(GTK_CONTAINER(user_data),widget);
}

static void
OnvifMgrMsgDialog__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifMgrMsgDialog * self = ONVIFMGR_MSGDIALOG (object);
    OnvifMgrMsgDialogPrivate *priv = OnvifMgrMsgDialog__get_instance_private (self);
    gpointer ptrvalue;
    const char * strvalue;

    switch (prop_id){
        case PROP_ICON:
            ptrvalue = GTK_WIDGET(g_value_get_pointer (value));
            if(GTK_IS_CONTAINER(priv->image_handle)){
                gtk_container_foreach (GTK_CONTAINER (priv->image_handle), (GtkCallback)OnvifMgrMsgDialog__container_remove_swapped, priv->image_handle);
                if(ptrvalue)
                    gtk_container_add (GTK_CONTAINER (priv->image_handle), ptrvalue);
            }
            break;
        case PROP_MSG:
            strvalue = g_value_get_string (value);
            if(GTK_IS_CONTAINER(priv->image_handle)){
                gtk_label_set_label(GTK_LABEL(priv->lblmessage),strvalue);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrMsgDialog__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifMgrMsgDialog * self = ONVIFMGR_MSGDIALOG (object);
    OnvifMgrMsgDialogPrivate *priv = OnvifMgrMsgDialog__get_instance_private (self);
    switch (prop_id){
        case PROP_ICON:
            g_value_set_pointer (value, gtk_container_get_children(GTK_CONTAINER(priv->image_handle)));
            break;
        case PROP_MSG:
            g_value_set_string (value, gtk_label_get_label(GTK_LABEL(priv->lblmessage)));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrMsgDialog__class_init (OnvifMgrMsgDialogClass *klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifMgrMsgDialog__set_property;
    object_class->get_property = OnvifMgrMsgDialog__get_property;
    OnvifMgrAppDialogClass * app_class = ONVIFMGR_APPDIALOG_CLASS (klass);
    app_class->create_ui = OnvifMgrMsgDialog__create_ui;

    obj_properties[PROP_ICON] = 
    g_param_spec_pointer ("icon", 
                        "Pointer to GtkImage", 
                        "Pointer to GtkImage to show as message icon",
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_MSG] =
        g_param_spec_string ("msg",
                            "Dialog message string",
                            "Message to display on dialog",
                            NULL,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

static void
OnvifMgrMsgDialog__init (OnvifMgrMsgDialog *self){
    g_object_set (self,
            "submit-label", MSG_DIALOG_SUBMIT_LABEL,
            NULL);
}

OnvifMgrMsgDialog * 
OnvifMgrMsgDialog__new(){
    return g_object_new (ONVIFMGR_TYPE_MSGDIALOG,
                        NULL);
}