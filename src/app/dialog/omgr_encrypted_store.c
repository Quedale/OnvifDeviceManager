#include "omgr_encrypted_store.h"
#include <pwd.h>
#include "../gui_utils.h"
#include "../../utils/encryption_utils.h"
#include "clogger.h"

#define ONVIFMGR_ENCRYPTEDSTORE_FILE_PATH "onvifmgr_store.bin"

enum
{
    PROP_QUEUE = 1,
    PROP_OVERLAY = 2,
    N_PROPERTIES
};

enum {
    NEW_OBJECT,
    CANCEL,
    LAST_SIGNAL
};

typedef struct {
    EventQueue * queue;
    GtkOverlay * overlay;
    gboolean store_exists;
    unsigned char * passphrase;
    int passphrase_len;
    OnvifMgrTrustStoreDialog * dialog;
} OnvifMgrEncryptedStorePrivate;

typedef struct _OnvifMgrEncryptedStoreClassPrivate {
    char store_path[PATH_MAX];
} OnvifMgrEncryptedStoreClassPrivate;

typedef struct _EncryptionChunkData {
    OnvifMgrEncryptedStore * store;
    unsigned char * data;
    int len_parsed;
    int len;
    int expected_len;
} EncryptionChunkData;

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

#define OMGR_ENCRYPTEDSTORE_INIT \
    g_type_add_class_private (g_define_type_id, \
        sizeof (OnvifMgrEncryptedStoreClassPrivate));

G_DEFINE_TYPE_WITH_CODE (OnvifMgrEncryptedStore, OnvifMgrEncryptedStore_, G_TYPE_OBJECT, 
                                G_ADD_PRIVATE (OnvifMgrEncryptedStore)
                                OMGR_ENCRYPTEDSTORE_INIT)

static gboolean 
OnvifMgrEncryptedStore__show_error(void * user_data){
    void ** arr = (void*)user_data;
    OnvifMgrAppDialog * self = ONVIFMGR_APPDIALOG(arr[0]);
    g_object_set(self,"error",arr[1],NULL);
    OnvifMgrAppDialog__hide_loading(self);

    free(user_data);
    return FALSE;
}

static void
build_object_callback(EncryptionChunkData * chunk){
    g_return_if_fail(chunk->data[0] == '[');

    int i=1;
    for(;i<chunk->len;i++){
        if(chunk->data[i] == ']'){
            chunk->data[i] = '\0';
            break;
        }
    }

    GType type = g_type_from_name((const char *)&chunk->data[1]);
    g_return_if_fail(type != 0);
    OnvifMgrSerializable * object = OnvifMgrSerializable__unserialize(type,&chunk->data[i+1],chunk->len-i-1);
    g_return_if_fail(object != NULL);

    gui_signal_emit(chunk->store, signals[NEW_OBJECT], object);
}

static int 
encrypted_chunk_callback (unsigned char * buffer, int buffer_length, void * user_data){
    EncryptionChunkData * chunk = (EncryptionChunkData *) user_data;

    int int_size = sizeof(int);
    int data_parsed = 0;
    while(data_parsed < buffer_length){
        if(!chunk->data){//Extract object size
            int * exp_len = &chunk->expected_len;
            int int_len_to_read = (data_parsed + int_size > buffer_length) ? buffer_length-data_parsed : (chunk->len_parsed > 0) ? int_size-chunk->len_parsed : int_size;

            memcpy(&exp_len[chunk->len_parsed],&buffer[data_parsed],int_len_to_read);
            chunk->len_parsed += int_len_to_read;
            data_parsed+= int_len_to_read;
    
            if(chunk->len_parsed == int_size){
                chunk->data = malloc(chunk->expected_len);
                chunk->len_parsed = 0;
            } else {
                continue;
            }
        }

        int len_to_read = (buffer_length-data_parsed >= chunk->expected_len-chunk->len) ? chunk->expected_len-chunk->len : buffer_length-data_parsed;

        if(len_to_read == 0) break; //Encryption adds extra \0 padding to chunks
        
        //Sanity check
        if(data_parsed + len_to_read > buffer_length || len_to_read < 0){
            C_WARN("Invalid serializble length detected.");
            return FALSE;
        }

        memcpy(&chunk->data[chunk->len],&buffer[data_parsed],len_to_read);
        data_parsed += len_to_read;
        chunk->len += len_to_read;

        if(chunk->expected_len == chunk->len){
            if(chunk->len >= 3)
                build_object_callback(chunk);
            chunk->expected_len = 0;
            chunk->len = 0;
            if(chunk->data)
                free(chunk->data);
            chunk->data = NULL;
        }
    }

    return TRUE;
}

static void
OnvifMgrEncryptedStore__dispose (GObject *object){
    OnvifMgrEncryptedStore * self = ONVIFMGR_ENCRYPTEDSTORE(object);
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    if(priv->passphrase){
        free(priv->passphrase);
        priv->passphrase = NULL;
        priv->passphrase_len = 0;
    }
    G_OBJECT_CLASS (OnvifMgrEncryptedStore__parent_class)->dispose (object);
}

static void 
OnvifMgrEncryptedStore__read_store(QueueEvent * qevt, void * user_data){
    OnvifMgrEncryptedStore * self = ONVIFMGR_ENCRYPTEDSTORE(user_data);
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    OnvifMgrEncryptedStoreClass * klass = ONVIFMGR_ENCRYPTEDSTORE_GET_CLASS(self);

    EncryptionChunkData floating_data;
    floating_data.data = NULL;
    floating_data.len = 0;
    floating_data.expected_len = 0;
    floating_data.store = self;
    floating_data.len_parsed = 0;
    int decrypted_data_len = EncryptionUtils__read_encrypted((unsigned char *) priv->passphrase,
                                    priv->passphrase_len,
                                    klass->extension->store_path,
                                    encrypted_chunk_callback, 
                                    &floating_data);

    if(floating_data.data)
        free(floating_data.data);

    if(decrypted_data_len <= 0){
        void ** input = malloc(sizeof(void*) *2);
        input[0] = priv->dialog;
        input[1] = "Failed to decrypt store file.";
        free(priv->passphrase);
        priv->passphrase = NULL;
        priv->passphrase_len = 0;
        g_idle_add(G_SOURCE_FUNC(OnvifMgrEncryptedStore__show_error),input);
        return;
    } else {
        g_idle_add(G_SOURCE_FUNC(OnvifMgrAppDialog__close),priv->dialog);
    }
}

static void 
OnvifMgrEncryptedStore__create_store(QueueEvent * qevt, void * user_data){
    OnvifMgrEncryptedStore* self = ONVIFMGR_ENCRYPTEDSTORE(user_data);
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    OnvifMgrEncryptedStoreClass * klass = ONVIFMGR_ENCRYPTEDSTORE_GET_CLASS(self);

    C_DEBUG("Creating store");

    if(access(klass->extension->store_path, F_OK) == 0){
        if(remove(klass->extension->store_path) == 0){
            C_DEBUG("Successfully removed previous store file...");
        } else {
            void ** input = malloc(sizeof(void*) *2);
            input[0] = priv->dialog;
            input[1] = "Failed to remove previous store file.";
            g_idle_add(G_SOURCE_FUNC(OnvifMgrEncryptedStore__show_error),input);
        }
    }
    char * plain_data = "[Empty File]";
    int plain_data_len = strlen(plain_data)+1;

    //TODO Validate passphrase strength
    int encrypted_len = EncryptionUtils__write_encrypted(priv->passphrase, priv->passphrase_len, (unsigned char *) plain_data, plain_data_len,klass->extension->store_path);
    if(encrypted_len <= 0){
        void ** input = malloc(sizeof(void*) *2);
        input[0] = self;
        input[1] = "Failed to create encypted file.";
        g_idle_add(G_SOURCE_FUNC(OnvifMgrEncryptedStore__show_error),input);
    } else {
        g_idle_add(G_SOURCE_FUNC(OnvifMgrAppDialog__close),priv->dialog);
    }
}

static gboolean 
OnvifMgrEncryptedStore__show_panel(void * user_data){
    OnvifMgrEncryptedStore * self = ONVIFMGR_ENCRYPTEDSTORE(user_data);
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);

    g_object_set (priv->dialog, "store-exists",priv->store_exists, NULL);
    OnvifMgrAppDialog__hide_loading(ONVIFMGR_APPDIALOG(priv->dialog));

    return FALSE;
}

static void 
OnvifMgrEncryptedStore__check_store_exists(QueueEvent * qevt, void * user_data){
    OnvifMgrEncryptedStore* self = ONVIFMGR_ENCRYPTEDSTORE(user_data);
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    OnvifMgrEncryptedStoreClass * klass = ONVIFMGR_ENCRYPTEDSTORE_GET_CLASS(self);
    
    priv->store_exists = access(klass->extension->store_path, F_OK) == 0;
    gdk_threads_add_idle(G_SOURCE_FUNC(OnvifMgrEncryptedStore__show_panel),self);
}

static void 
OnvifMgrEncryptedStore__reset(GtkWidget * widget, OnvifMgrEncryptedStore * self){
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    g_object_set(priv->dialog,"error",NULL,NULL);
    if(widget){
        OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(priv->dialog),"Resetting up encrypted store file...");
    } else {
        OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(priv->dialog),"Setting up encrypted store file...");
    }
    
    if(priv->passphrase) free(priv->passphrase);
    priv->passphrase_len = OnvifMgrTrustStoreDialog__get_passphrase_len(priv->dialog);
    priv->passphrase = malloc(priv->passphrase_len);
    memcpy(priv->passphrase,OnvifMgrTrustStoreDialog__get_passphrase(priv->dialog),priv->passphrase_len);

    EventQueue__insert(priv->queue, self, OnvifMgrEncryptedStore__create_store,self, NULL);
}

static void 
OnvifMgrEncryptedStore__login(OnvifMgrTrustStoreDialog * dialog, OnvifMgrEncryptedStore * self){
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    OnvifMgrEncryptedStoreClass * klass = ONVIFMGR_ENCRYPTEDSTORE_GET_CLASS(self);
    if(priv->store_exists){
        C_DEBUG("Reading existing store. '%s'",klass->extension->store_path);
        g_object_set(priv->dialog,"error",NULL,NULL);

        OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(priv->dialog),"Decrypting store file...");
        
        //Extract from GUI before starting background thread
        if(priv->passphrase) free(priv->passphrase);
        priv->passphrase_len = OnvifMgrTrustStoreDialog__get_passphrase_len(priv->dialog);
        priv->passphrase = malloc(priv->passphrase_len);
        memcpy(priv->passphrase,OnvifMgrTrustStoreDialog__get_passphrase(priv->dialog),priv->passphrase_len);

        EventQueue__insert(priv->queue, self, OnvifMgrEncryptedStore__read_store,self, NULL);
    } else {
        C_DEBUG("Creating new store. '%s'",klass->extension->store_path);
        OnvifMgrEncryptedStore__reset(NULL,self);
    }
}

static void 
OnvifMgrEncryptedStore__cancel(OnvifMgrTrustStoreDialog * dialog, OnvifMgrEncryptedStore * self){
    //TODO Show confirmation message
    g_signal_emit(self, signals[CANCEL], 0, NULL);
}

static void
OnvifMgrEncryptedStore__showing (GtkWidget *widget, OnvifMgrEncryptedStore * self){
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    OnvifMgrAppDialog__show_loading(ONVIFMGR_APPDIALOG(widget),"Checking if encrypted store file exists...");
    EventQueue__insert(priv->queue, self, OnvifMgrEncryptedStore__check_store_exists,self, NULL);
}

void
OnvifMgrEncryptedStore__capture_passphrase(OnvifMgrEncryptedStore * self){
    g_return_if_fail (self != NULL);
    g_return_if_fail (ONVIFMGR_IS_ENCRYPTEDSTORE (self));

    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    g_return_if_fail (priv->overlay != NULL);
    g_return_if_fail (GTK_IS_OVERLAY(priv->overlay));

    g_return_if_fail (priv->queue != NULL);
    g_return_if_fail (QUEUE_IS_EVENTQUEUE(priv->queue));

    priv->dialog = OnvifMgrTrustStoreDialog__new(priv->queue);
    g_signal_connect (G_OBJECT (priv->dialog), "showing", G_CALLBACK (OnvifMgrEncryptedStore__showing), self);
    g_signal_connect (G_OBJECT (priv->dialog), "submit", G_CALLBACK (OnvifMgrEncryptedStore__login), self);
    g_signal_connect (G_OBJECT (priv->dialog), "reset", G_CALLBACK (OnvifMgrEncryptedStore__reset), self);
    g_signal_connect (G_OBJECT (priv->dialog), "cancel", G_CALLBACK (OnvifMgrEncryptedStore__cancel), self);

    gtk_overlay_add_overlay(priv->overlay,GTK_WIDGET(priv->dialog));
    gtk_widget_show_all(GTK_WIDGET(priv->overlay));

}

static void
OnvifMgrEncryptedStore__set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec){
    OnvifMgrEncryptedStore * self = ONVIFMGR_ENCRYPTEDSTORE (object);
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    EventQueue * queue;
    GtkOverlay * overlay;

    switch (prop_id){
        case PROP_QUEUE:
            queue = g_value_get_object (value);
            g_return_if_fail (queue != NULL);
            g_return_if_fail (QUEUE_IS_EVENTQUEUE(queue));
            priv->queue = queue;
            break;
        case PROP_OVERLAY:
            overlay = g_value_get_object (value);
            g_return_if_fail (overlay != NULL);
            g_return_if_fail (GTK_IS_OVERLAY(overlay));
            priv->overlay = overlay;
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
OnvifMgrEncryptedStore__get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec){
    OnvifMgrEncryptedStore * self = ONVIFMGR_ENCRYPTEDSTORE (object);
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    switch (prop_id){
        case PROP_QUEUE:
            g_value_set_object (value, priv->queue);
            break;
        case PROP_OVERLAY:
            g_value_set_object (value, priv->overlay);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void 
OnvifMgrEncryptedStore__get_config_path(char * path){
    const char * configdir;
    if((configdir = getenv("XDG_CONFIG_HOME")) != NULL){
        strcpy(path,configdir);
        strcat(path,"/");
        strcat(path,ONVIFMGR_ENCRYPTEDSTORE_FILE_PATH);

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
    strcat(path,ONVIFMGR_ENCRYPTEDSTORE_FILE_PATH);

    if(buf) free(buf);
}

static void
OnvifMgrEncryptedStore__class_init (OnvifMgrEncryptedStoreClass *klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = OnvifMgrEncryptedStore__set_property;
    object_class->get_property = OnvifMgrEncryptedStore__get_property;
    object_class->dispose = OnvifMgrEncryptedStore__dispose;
    g_type_class_ref (ONVIFMGR_TYPE_ENCRYPTEDSTORE);

    klass->extension = G_TYPE_CLASS_GET_PRIVATE (klass,
      ONVIFMGR_TYPE_ENCRYPTEDSTORE, OnvifMgrEncryptedStoreClassPrivate);

    GType params[1];
    params[0] = OMGR_TYPE_SERIALIZABLE | G_SIGNAL_TYPE_STATIC_SCOPE;
    signals[NEW_OBJECT] =
        g_signal_newv ("new-object",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        1     /* n_params */,
                        params  /* param_types */);

    signals[CANCEL] =
        g_signal_newv ("cancel",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                        NULL /* closure */,
                        NULL /* accumulator */,
                        NULL /* accumulator data */,
                        NULL /* C marshaller */,
                        G_TYPE_NONE /* return_type */,
                        0     /* n_params */,
                        NULL  /* param_types */);         

    obj_properties[PROP_QUEUE] =
        g_param_spec_object ("queue",
                            "EventQueue",
                            "Pointer to EventQueue parent.",
                            QUEUE_TYPE_EVENTQUEUE,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    obj_properties[PROP_OVERLAY] =
        g_param_spec_object ("overlay",
                            "GtkOverlay",
                            "Pointer to GtkOverlay to display dialog into.",
                            GTK_TYPE_OVERLAY,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    OnvifMgrEncryptedStore__get_config_path(klass->extension->store_path);

    g_object_class_install_properties (object_class,
                                        N_PROPERTIES,
                                        obj_properties);
}

static void
OnvifMgrEncryptedStore__init (OnvifMgrEncryptedStore *self){
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    priv->queue = NULL;
    priv->overlay = NULL;
    priv->store_exists = FALSE;
    priv->passphrase = NULL;
    priv->passphrase_len = 0;
    priv->dialog = NULL;
}

OnvifMgrEncryptedStore * 
OnvifMgrEncryptedStore__new(EventQueue * queue, GtkOverlay * overlay){
    return g_object_new (ONVIFMGR_TYPE_ENCRYPTEDSTORE,
                        "queue",queue,
                        "overlay",overlay,
                        NULL);
}

int
OnvifMgrEncryptedStore__save (OnvifMgrEncryptedStore *self, GList * data){
    g_return_val_if_fail (self != NULL, -1);
    g_return_val_if_fail (ONVIFMGR_IS_ENCRYPTEDSTORE (self),-1);
    OnvifMgrEncryptedStorePrivate *priv = OnvifMgrEncryptedStore__get_instance_private (self);
    OnvifMgrEncryptedStoreClass * klass = ONVIFMGR_ENCRYPTEDSTORE_GET_CLASS(self);

    unsigned char * data_to_write = NULL;
    int data_to_write_len = 0;
    OnvifMgrSerializable * serializable;
    GLIST_FOREACH(serializable, data) {
        int serialized_length;
        unsigned char * data = OnvifMgrSerializable__serialize(serializable, &serialized_length);
        if(!data) {
            C_WARN("Serialized data is NULL");
            continue;
        }

        const char * typestr = g_type_name(G_TYPE_FROM_INSTANCE(serializable));
        int typestr_len = strlen(typestr);
        int charsize = sizeof(char);
        int intsize = sizeof(int);
        int datalen = serialized_length + typestr_len + charsize*2;
        if(!data_to_write){
            data_to_write = malloc(intsize+datalen);
        } else {
            data_to_write = realloc(data_to_write,data_to_write_len+intsize+datalen);
        }
        memcpy (&data_to_write[data_to_write_len], &datalen, intsize);
        memcpy (&data_to_write[data_to_write_len+intsize], "[", charsize);
        memcpy (&data_to_write[data_to_write_len+intsize +charsize], typestr, typestr_len);
        memcpy (&data_to_write[data_to_write_len+intsize+charsize+typestr_len], "]", charsize);
        memcpy (&data_to_write[data_to_write_len+intsize+charsize+typestr_len+charsize], data, serialized_length);
        data_to_write_len += (intsize+datalen);
        free(data);
    }

    g_return_val_if_fail(data_to_write != NULL, -1);

    int encrypted_len = EncryptionUtils__write_encrypted(priv->passphrase, priv->passphrase_len, data_to_write, data_to_write_len,klass->extension->store_path);
    free(data_to_write);
    return encrypted_len;
}
