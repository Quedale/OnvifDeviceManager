/**
 * 
 * Interface to use when an object is shared accross multiple thread branches
 * Using ref_count alone isn't enough since multiple child thread may keep a reference without being the owner
 * 
 * This way, changing the ownership state allows to flag child threads that this object is no longer valid
 * 
 * Each implementation can choose to make it thread-safe or not.
 * 
*/

#ifndef ONVIFMGR_SERIALIZABLE_INTERFACE_H_ 
#define ONVIFMGR_SERIALIZABLE_INTERFACE_H_


#include <glib-object.h>

G_BEGIN_DECLS

#define OMGR_TYPE_SERIALIZABLE OnvifMgrSerializable__get_type()
G_DECLARE_INTERFACE (OnvifMgrSerializable, OnvifMgrSerializable_, OMGR, SERIALIZABLE, GObject)

struct _OnvifMgrSerializableInterface
{
  GTypeInterface parent_iface;
  int (*serialize) (OnvifMgrSerializable  *self, unsigned char * output);
  OnvifMgrSerializable * (*unserialize) (unsigned char * data, int length);
};


struct _OnvifMgrSerializableInterfaceClass
{
  GObjectClass parent_class;
};

int OnvifMgrSerializable__serialize(OnvifMgrSerializable * self, unsigned char * output);
OnvifMgrSerializable * OnvifMgrSerializable__unserialize(GType type, unsigned char * data, int length);

G_END_DECLS

#endif