#include "omgr_truststore_dialog.h"
#include "../../utils/encryption_utils.h"
#include "clogger.h"
#include <pwd.h>

#define ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_LOGIN_LABEL "Login"
#define ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_CREATE_LABEL "Create"
#define ONVIFMGR_TRUSTSTOR_DIALOG_LOGIN_TITLE "Credentials Store Authentication"
#define ONVIFMGR_TRUSTSTOR_DIALOG_CREATE_TITLE "Credentials Store Setup"
#define ONVIFMGR_TRUSTSTOR_DIALOG_DEFAULT_TITLE "Credentials Store Check"
#define ONVIFMGR_TRUSTSTOR_FILE_PATH "onvifmgr_store.bin"

enum
{
    PROP_QUEUE = 1,
    N_PROPERTIES
};

typedef struct {
    GtkWidget * txtkey;
    GtkWidget * btn_reset;
    char path[PATH_MAX];
    gboolean store_exists;
    EventQueue * queue;
} OnvifMgrTrustStoreDialogPrivate;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static unsigned char * application_store_passphrase = NULL;
static int application_store_passphrase_len = 0;

G_DEFINE_TYPE_WITH_PRIVATE (OnvifMgrTrustStoreDialog, OnvifMgrTrustStoreDialog_, ONVIFMGR_TYPE_APPDIALOG)

static gboolean 
OnvifMgrTrustStoreDialog__show_error(void * user_data){
    void ** arr = (void*)user_data;
    OnvifMgrAppDialog * self = ONVIFMGR_APPDIALOG(arr[0]);
    g_object_set(self,"error",arr[1],NULL);
    OnvifMgrAppDialog__hide_loading(self);

    free(user_data);
    return FALSE;
}

static void 
OnvifMgrTrustStoreDialog__create_store(QueueEvent * qevt, void * user_data){
    OnvifMgrTrustStoreDialog* self = ONVIFMGR_TRUSTSTOREDIALOG(user_data);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    C_DEBUG("Creating store");

    if(access(priv->path, F_OK) == 0){
        if(remove(priv->path) == 0){
            C_DEBUG("Successfully removed previous store file...");
        } else {
            void ** input = malloc(sizeof(void*) *2);
            input[0] = self;
            input[1] = "Failed to remove previous store file.";
            g_idle_add(G_SOURCE_FUNC(OnvifMgrTrustStoreDialog__show_error),input);
        }
    }
    char * plain_data = "[Empty File]";
    int plain_data_len = strlen(plain_data)+1;

    //TODO Validate passphrase strength
    int encrypted_len = EncryptionUtils__write_encrypted(application_store_passphrase, application_store_passphrase_len, (unsigned char *) plain_data, plain_data_len,priv->path);
    if(encrypted_len <= 0){
        void ** input = malloc(sizeof(void*) *2);
        input[0] = self;
        input[1] = "Failed to create encypted file.";
        g_idle_add(G_SOURCE_FUNC(OnvifMgrTrustStoreDialog__show_error),input);
    } else {
        g_idle_add(G_SOURCE_FUNC(OnvifMgrAppDialog__close),self);
    }
}

static void 
OnvifMgrTrustStoreDialog__reset(GtkWidget * widget, OnvifMgrTrustStoreDialog * self){
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    g_object_set(self,"error",NULL,NULL);
    if(widget){
        OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(self),"Resetting up encrypted store file...");
    } else {
        OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(self),"Setting up encrypted store file...");
    }
    application_store_passphrase = (unsigned char *) gtk_entry_get_text(GTK_ENTRY(priv->txtkey));
    application_store_passphrase_len = gtk_entry_get_text_length(GTK_ENTRY(priv->txtkey));
    EventQueue__insert(priv->queue, self, OnvifMgrTrustStoreDialog__create_store,self, NULL);
}

static int 
encrypted_chunk_callback (unsigned char * buffer, int buffer_length, void * user_data){

    /*
        The caller can process the chunk and copy the remaining data into user_data pointer.
        The remaining data will be available on the next invokation for further processing
    */

    //Print chunk decrypted with added null terminating character (DEBUG)
    unsigned char null_term_buffer[buffer_length+1];
    memcpy(null_term_buffer, buffer, buffer_length);
    null_term_buffer[buffer_length] = '\0';
    C_DEBUG("Chunk decrypted '%s'",null_term_buffer);

    //Return false if processing failed to abort
    return TRUE;
}

static void 
OnvifMgrTrustStoreDialog__read_store(QueueEvent * qevt, void * user_data){
    OnvifMgrTrustStoreDialog* self = ONVIFMGR_TRUSTSTOREDIALOG(user_data);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);

    int decrypted_data_len = EncryptionUtils__read_encrypted(application_store_passphrase,
                                    application_store_passphrase_len,
                                    priv->path,
                                    encrypted_chunk_callback, 
                                    NULL);
    if(decrypted_data_len <= 0){
        void ** input = malloc(sizeof(void*) *2);
        input[0] = self;
        input[1] = "Failed to decrypt store file.";
        g_idle_add(G_SOURCE_FUNC(OnvifMgrTrustStoreDialog__show_error),input);
        return;
    } else {
        g_idle_add(G_SOURCE_FUNC(OnvifMgrAppDialog__close),self);
    }
}

static void 
OnvifMgrTrustStoreDialog__login(OnvifMgrTrustStoreDialog * self){
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    if(priv->store_exists){
        C_DEBUG("Reading existing store. '%s'",priv->path);
        g_object_set(self,"error",NULL,NULL);
        application_store_passphrase = (unsigned char *) gtk_entry_get_text(GTK_ENTRY(priv->txtkey));
        application_store_passphrase_len = gtk_entry_get_text_length(GTK_ENTRY(priv->txtkey));
        OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(self),"Decrypting store file...");
        EventQueue__insert(priv->queue, self, OnvifMgrTrustStoreDialog__read_store,self, NULL);
    } else {
        C_DEBUG("Creating new store. '%s'",priv->path);
        OnvifMgrTrustStoreDialog__reset(NULL,self);
    }
}

static GtkWidget * 
OnvifMgrTrustStoreDialog__create_ui(OnvifMgrAppDialog * app_dialog){
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (ONVIFMGR_TRUSTSTOREDIALOG(app_dialog));
    GtkWidget * grid = gtk_grid_new ();
    gtk_grid_set_column_spacing(GTK_GRID (grid),10);

    GtkWidget * label = gtk_label_new("Store key:");
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
    g_signal_connect (G_OBJECT(priv->btn_reset), "clicked", G_CALLBACK (OnvifMgrTrustStoreDialog__reset), app_dialog);
    OnvifMgrAppDialog__add_action_widget(app_dialog, priv->btn_reset, ONVIFMGR_APPDIALOG_BUTTON_MIDDLE);

    g_object_set (app_dialog,
            "title-label",ONVIFMGR_TRUSTSTOR_DIALOG_DEFAULT_TITLE,
            NULL);

    g_signal_connect (G_OBJECT (app_dialog), "submit", G_CALLBACK (OnvifMgrTrustStoreDialog__login), NULL);
    //Cancel triggers system exit (registered by OnvifApp)
    // g_signal_connect (app_dialog, "cancel", G_CALLBACK (OnvifApp__cred_dialog_cancel_cb), app_dialog);

    return grid;
}

static void
OnvifMgrTrustStoreDialog__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifMgrTrustStoreDialog * self = ONVIFMGR_TRUSTSTOREDIALOG (object);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);

    switch (prop_id){
        case PROP_QUEUE:
            priv->queue = g_value_get_object (value);
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
        case PROP_QUEUE:
            g_value_set_object (value, priv->queue);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static gboolean 
OnvifMgrTrustStoreDialog__show_panel(void * user_data){
    OnvifMgrTrustStoreDialog * self = ONVIFMGR_TRUSTSTOREDIALOG(user_data);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);

    if(priv->store_exists){
        C_DEBUG("Credentials Store found. '%s'",priv->path);
        g_object_set (user_data,
                "title-label",ONVIFMGR_TRUSTSTOR_DIALOG_LOGIN_TITLE,
                "submit-label", ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_LOGIN_LABEL,
                NULL);
        gtk_widget_set_visible(priv->btn_reset,TRUE);
    } else {
        C_DEBUG("Credentials Store not found. '%s'",priv->path);
        g_object_set (user_data,
                "title-label",ONVIFMGR_TRUSTSTOR_DIALOG_CREATE_TITLE,
                "submit-label", ONVIFMGR_TRUSTSTOR_DIALOG_SUBMIT_CREATE_LABEL,
                NULL);
        gtk_widget_set_visible(priv->btn_reset,FALSE);
    }
    
    OnvifMgrAppDialog__hide_loading(ONVIFMGR_APPDIALOG(self));

    return FALSE;
}

static void 
AppSettings__get_config_path(char * path){
    const char * configdir;
    if((configdir = getenv("XDG_CONFIG_HOME")) != NULL){
        strcpy(path,configdir);
        strcat(path,"/");
        strcat(path,ONVIFMGR_TRUSTSTOR_FILE_PATH);

        C_TRACE("Using XDG_CONFIG_HOME config directory : %s",configdir);
        return;
    }

    const char *homedir;
    char *buf = NULL;
    if ((homedir = getenv("HOME")) == NULL) {
        struct passwd pwd;
        struct passwd *result;
        size_t bufsize;
        int s;
        bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (bufsize == (unsigned int) -1)
            bufsize = 0x4000; // = all zeroes with the 14th bit set (1 << 14)
        buf = malloc(bufsize);
        if (buf == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        s = getpwuid_r(getuid(), &pwd, buf, bufsize, &result);
        if (result == NULL) {
            if (s == 0)
                printf("Not found\n");
            else {
                errno = s;
                perror("getpwnam_r");
            }
            exit(EXIT_FAILURE);
        }
        homedir = result->pw_dir;
    }

    C_TRACE("Generating default config path from HOME directory : %s\n",homedir);

    strcpy(path,homedir);
    strcat(path,"/.config/");
    strcat(path,ONVIFMGR_TRUSTSTOR_FILE_PATH);

    if(buf) free(buf);
}

static void 
OnvifMgrTrustStoreDialog__check_store_exists(QueueEvent * qevt, void * user_data){
    OnvifMgrTrustStoreDialog* self = ONVIFMGR_TRUSTSTOREDIALOG(user_data);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);

    AppSettings__get_config_path(priv->path);
    priv->store_exists = access(priv->path, F_OK) == 0;

    gdk_threads_add_idle(G_SOURCE_FUNC(OnvifMgrTrustStoreDialog__show_panel),self);
}

static void
OnvifMgrTrustStoreDialog__show (GtkWidget *widget){
    OnvifMgrTrustStoreDialog * self = ONVIFMGR_TRUSTSTOREDIALOG(widget);
    OnvifMgrTrustStoreDialogPrivate *priv = OnvifMgrTrustStoreDialog__get_instance_private (self);
    gtk_entry_set_text(GTK_ENTRY(priv->txtkey),"");
    OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(self),"Checking if encrypted store file exists...");
    EventQueue__insert(priv->queue, self, OnvifMgrTrustStoreDialog__check_store_exists,self, NULL);
}

static void
OnvifMgrTrustStoreDialog__class_init (OnvifMgrTrustStoreDialogClass *klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifMgrTrustStoreDialog__set_property;
    object_class->get_property = OnvifMgrTrustStoreDialog__get_property;
    OnvifMgrAppDialogClass * app_class = ONVIFMGR_APPDIALOG_CLASS (klass);
    app_class->create_ui = OnvifMgrTrustStoreDialog__create_ui;
    app_class->show = OnvifMgrTrustStoreDialog__show;

    obj_properties[PROP_QUEUE] =
        g_param_spec_object ("queue",
                            "EventQueue",
                            "Pointer to EventQueue parent.",
                            QUEUE_TYPE_EVENTQUEUE,~
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

static void
OnvifMgrTrustStoreDialog__init (OnvifMgrTrustStoreDialog *self){

}

OnvifMgrTrustStoreDialog * 
OnvifMgrTrustStoreDialog__new(EventQueue * queue){
    return g_object_new (ONVIFMGR_TYPE_TRUSTSTOREDIALOG,
                        "queue",queue,
                        NULL);
}