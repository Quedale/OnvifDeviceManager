#include "c_ownable_interface.h"


G_DEFINE_INTERFACE (COwnableObject, COwnableObject_, G_TYPE_OBJECT)

static void
COwnableObject__default_init (COwnableObjectInterface *iface)
{
    /* add properties and signals to the interface here */
}

gboolean COwnableObject__has_owner(COwnableObject * self){
    COwnableObjectInterface *iface;

    g_return_val_if_fail (COWNABLE_IS_OBJECT (self),FALSE);

    iface = COWNABLE_OBJECT_GET_IFACE (self);
    g_return_val_if_fail (iface->has_owner != NULL,FALSE);
    return iface->has_owner (self);
}
void COwnableObject__disown(COwnableObject * self){
    COwnableObjectInterface *iface;
    
    g_return_if_fail (COWNABLE_IS_OBJECT (self));

    iface = COWNABLE_OBJECT_GET_IFACE (self);
    g_return_if_fail (iface->has_owner != NULL);
    iface->disown (self);
}
