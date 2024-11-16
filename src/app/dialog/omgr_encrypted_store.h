#ifndef ONVIFMGR_ENCRYPTEDSTORE_
#define ONVIFMGR_ENCRYPTEDSTORE_

#include <gtk/gtk.h>
#include "omgr_encryoted_store_dialog.h"

G_BEGIN_DECLS
typedef struct _OnvifMgrEncryptedStore OnvifMgrEncryptedStore;
typedef struct _OnvifMgrEncryptedStoreClass OnvifMgrEncryptedStoreClass;
typedef struct _OnvifMgrEncryptedStoreClassPrivate OnvifMgrEncryptedStoreClassPrivate;

//Expanded definition to support private class extension
#define ONVIFMGR_TYPE_ENCRYPTEDSTORE OnvifMgrEncryptedStore__get_type()
#define ONVIFMGR_ENCRYPTEDSTORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),ONVIFMGR_TYPE_ENCRYPTEDSTORE,OnvifMgrEncryptedStore))
#define ONVIFMGR_ENCRYPTEDSTORE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),ONVIFMGR_TYPE_ENCRYPTEDSTORE,OnvifMgrEncryptedStoreClass))
#define ONVIFMGR_ENCRYPTEDSTORE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),ONVIFMGR_TYPE_ENCRYPTEDSTORE,OnvifMgrEncryptedStoreClass))
#define ONVIFMGR_IS_ENCRYPTEDSTORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),ONVIFMGR_TYPE_ENCRYPTEDSTORE))
#define ONVIFMGR_IS_ENCRYPTEDSTORE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),ONVIFMGR_TYPE_ENCRYPTEDSTORE))

struct _OnvifMgrEncryptedStore
{
    GObject parent;
};

struct _OnvifMgrEncryptedStoreClass
{
    GObjectClass parent;
    OnvifMgrEncryptedStoreClassPrivate * extension;
};

OnvifMgrEncryptedStore * OnvifMgrEncryptedStore__new(EventQueue * queue, GtkOverlay * overlay);
void OnvifMgrEncryptedStore__capture_passphrase(OnvifMgrEncryptedStore * self);
int OnvifMgrEncryptedStore__save (OnvifMgrEncryptedStore *self, GList * data);

G_END_DECLS

#endif