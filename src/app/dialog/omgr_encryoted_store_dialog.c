#include "omgr_encryoted_store_dialog.h"
#include "clogger.h"

#define ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_LOGIN_LABEL "Login"
#define ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_CREATE_LABEL "Create"
#define ONVIFMGR_TRUSTSTOR_DIALOG_LOGIN_TITLE "Credentials Store Authentication"
#define ONVIFMGR_TRUSTSTOR_DIALOG_CREATE_TITLE "Credentials Store Setup"
#define ONVIFMGR_TRUSTSTOR_DIALOG_DEFAULT_TITLE "Credentials Store Check"
#define ONVIFMGR_TRUSTSTOR_FILE_PATH "onvifmgr_store.bin"

enum
{
    PROP_STOREEXISTS = 1,
    N_PROPERTIES
};

enum {
    NEW_OBJECT,
    RESET,
    LAST_SIGNAL
};

typedef struct {
    GtkWidget * txtkey;
    GtkWidget * btn_reset;
} OnvifMgrTrustStoreDialogPrivate;

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (OnvifMgrTrustStoreDialog, OnvifMgrTrustStoreDialog_, ONVIFMGR_TYPE_APPDIALOG)

static void 
OnvifMgrTrustStoreDialog__reset(GtkWidget * widget, OnvifMgrTrustStoreDialog * self){
    g_signal_emit(self, signals[RESET], 0, NULL);
}

static GtkWidget * 
OnvifMgrTrustStoreDialog__create_ui(OnvifMgrAppDialog * app_dialog){
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (ONVIFMGR_TRUSTSTOREDIALOG(app_dialog));
    GtkWidget * grid = gtk_grid_new ();
    gtk_grid_set_column_spacing(GTK_GRID (grid),10);

    GtkWidget * label = gtk_label_new("Passphrase:");
    gtk_label_set_line_wrap(GTK_LABEL(label),1);
    gtk_widget_set_hexpand (label, FALSE);
    gtk_widget_set_vexpand (label, FALSE);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);

    priv->txtkey = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(priv->txtkey),FALSE);
    gtk_widget_set_hexpand (priv->txtkey, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->txtkey, 1, 0, 1, 1);

    priv->btn_reset = gtk_button_new_with_label("Reset");
    gtk_widget_set_visible(priv->btn_reset,FALSE); //Default value
    g_signal_connect (G_OBJECT(priv->btn_reset), "clicked", G_CALLBACK (OnvifMgrTrustStoreDialog__reset), app_dialog);
    OnvifMgrAppDialog__add_action_widget(app_dialog, priv->btn_reset, ONVIFMGR_APPDIALOG_BUTTON_MIDDLE);

    g_object_set (app_dialog,
            "title-label",ONVIFMGR_TRUSTSTOR_DIALOG_DEFAULT_TITLE,
            NULL);
    
    return grid;
}

static void
OnvifMgrTrustStoreDialog__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifMgrTrustStoreDialog * self = ONVIFMGR_TRUSTSTOREDIALOG (object);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    gboolean boolval;

    switch (prop_id){
        case PROP_STOREEXISTS:
            boolval = g_value_get_boolean (value);
            if(boolval){
                g_object_set (object,
                    "title-label",ONVIFMGR_TRUSTSTOR_DIALOG_LOGIN_TITLE,
                    "submit-label", ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_LOGIN_LABEL,
                    NULL);
            } else {
                g_object_set (object,
                        "title-label",ONVIFMGR_TRUSTSTOR_DIALOG_CREATE_TITLE,
                        "submit-label", ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_CREATE_LABEL,
                        NULL);
            }
            gtk_widget_set_visible(priv->btn_reset,boolval);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrTrustStoreDialog__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifMgrTrustStoreDialog * self = ONVIFMGR_TRUSTSTOREDIALOG (object);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    switch (prop_id){
        case PROP_STOREEXISTS:
            g_value_set_boolean (value, gtk_widget_is_visible(priv->btn_reset));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

const char * 
OnvifMgrTrustStoreDialog__get_passphrase(OnvifMgrTrustStoreDialog * self){
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (ONVIFMGR_IS_TRUSTSTOREDIALOG (self),NULL);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    return gtk_entry_get_text(GTK_ENTRY(priv->txtkey));
}

int 
OnvifMgrTrustStoreDialog__get_passphrase_len(OnvifMgrTrustStoreDialog * self){
    g_return_val_if_fail (self != NULL, 0);
    g_return_val_if_fail (ONVIFMGR_IS_TRUSTSTOREDIALOG (self),0);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    return gtk_entry_get_text_length(GTK_ENTRY(priv->txtkey));
}


static void
OnvifMgrTrustStoreDialog__showing (GtkWidget *widget){
    OnvifMgrTrustStoreDialog * self = ONVIFMGR_TRUSTSTOREDIALOG(widget);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    gtk_entry_set_text(GTK_ENTRY(priv->txtkey),"");
}

static void
OnvifMgrTrustStoreDialog__class_init (OnvifMgrTrustStoreDialogClass *klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifMgrTrustStoreDialog__set_property;
    object_class->get_property = OnvifMgrTrustStoreDialog__get_property;
    OnvifMgrAppDialogClass * app_class = ONVIFMGR_APPDIALOG_CLASS (klass);
    app_class->create_ui = OnvifMgrTrustStoreDialog__create_ui;
    app_class->showing = OnvifMgrTrustStoreDialog__showing;


    signals[RESET] =
        g_signal_newv ("reset",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        0     /* n_params */,
                        NULL  /* param_types */);

    obj_properties[PROP_STOREEXISTS] =
        g_param_spec_boolean ("store-exists",
                            "StoreFileExists",
                            "gboolean flag to set if the store file exists on the system",
                            FALSE,
                            G_PARAM_READWRITE);

    
    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

static void
OnvifMgrTrustStoreDialog__init (OnvifMgrTrustStoreDialog *self){

}

OnvifMgrTrustStoreDialog * 
OnvifMgrTrustStoreDialog__new(){
    return g_object_new (ONVIFMGR_TYPE_TRUSTSTOREDIALOG, NULL);
}