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

#ifndef C_OWNABLE_OBJECT_H_ 
#define C_OWNABLE_OBJECT_H_


#include <glib-object.h>

G_BEGIN_DECLS

#define COWNABLE_TYPE_OBJECT COwnableObject__get_type()
G_DECLARE_INTERFACE (COwnableObject, COwnableObject_, COWNABLE, OBJECT, GObject)

struct _COwnableObjectInterface
{
  GTypeInterface parent_iface;
  gboolean (*has_owner) (COwnableObject  *self);
  void (*disown) (COwnableObject  *self);
};


struct _COwnableObjectInterfaceClass
{
  GObjectClass parent_class;
};

gboolean COwnableObject__has_owner(COwnableObject * self);
void COwnableObject__disown(COwnableObject * self);

G_END_DECLS

#endif