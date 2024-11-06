#include "omgr_truststore_dialog.h"
#include "clogger.h"

#define ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_LABEL "Login"

// enum
// {
//   N_PROPERTIES
// };

typedef struct {
    GtkWidget * txtkey;
    GtkWidget * btn_reset;
} OnvifMgrTrustStoreDialogPrivate;

// static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (OnvifMgrTrustStoreDialog, OnvifMgrTrustStoreDialog_, ONVIFMGR_TYPE_APPDIALOG)

static GtkWidget * 
OnvifMgrTrustStoreDialog__create_ui(OnvifMgrAppDialog * app_dialog){
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (ONVIFMGR_TRUSTSTOREDIALOG(app_dialog));
    GtkWidget * grid = gtk_grid_new ();
    gtk_grid_set_column_spacing(GTK_GRID (grid),10);
    // gtk_widget_set_margin_start(grid,10);
    // gtk_widget_set_margin_end(grid,10);
    GtkWidget * label = gtk_label_new("TrustStore key:");
    gtk_label_set_line_wrap(GTK_LABEL(label),1);
    gtk_widget_set_hexpand (label, FALSE);
    gtk_widget_set_vexpand (label, FALSE);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);

    priv->txtkey = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(priv->txtkey),FALSE);
    gtk_widget_set_hexpand (priv->txtkey, TRUE);
    gtk_grid_attach (GTK_GRID (grid), priv->txtkey, 1, 0, 1, 1);

    GtkWidget * reset_button = gtk_button_new_with_label("Reset");
    OnvifMgrAppDialog__add_action_widget(app_dialog, reset_button, ONVIFMGR_APPDIALOG_BUTTON_MIDDLE);

    g_object_set (app_dialog,
            "submit-label", ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_LABEL,
            NULL);

    return grid;
}

static void
OnvifMgrTrustStoreDialog__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    // OnvifMgrTrustStoreDialog * self = ONVIFMGR_TRUSTSTOREDIALOG (object);
    // OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    // gpointer ptrvalue;
    // const char * strvalue;

    switch (prop_id){
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
    // OnvifMgrTrustStoreDialog * self = ONVIFMGR_TRUSTSTOREDIALOG (object);
    // OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    switch (prop_id){
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrTrustStoreDialog__class_init (OnvifMgrTrustStoreDialogClass *klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifMgrTrustStoreDialog__set_property;
    object_class->get_property = OnvifMgrTrustStoreDialog__get_property;
    OnvifMgrAppDialogClass * app_class = ONVIFMGR_APPDIALOG_CLASS (klass);
    app_class->create_ui = OnvifMgrTrustStoreDialog__create_ui;


    // g_object_class_install_properties (object_class,
    //                                     N_PROPERTIES,
    //                                     obj_properties);
}

static void
OnvifMgrTrustStoreDialog__init (OnvifMgrTrustStoreDialog *self){

}

OnvifMgrTrustStoreDialog * 
OnvifMgrTrustStoreDialog__new(){
    return g_object_new (ONVIFMGR_TYPE_TRUSTSTOREDIALOG,
                        NULL);
}