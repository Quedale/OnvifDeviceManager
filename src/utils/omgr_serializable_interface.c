#include "omgr_serializable_interface.h"
#include "clogger.h"

G_DEFINE_INTERFACE (OnvifMgrSerializable, OnvifMgrSerializable_, G_TYPE_OBJECT)

static void
OnvifMgrSerializable__default_init (OnvifMgrSerializableInterface *iface)
{
    iface->serialize = NULL;
    iface->unserialize = NULL;
}

OnvifMgrSerializable * OnvifMgrSerializable__unserialize(GType type, unsigned char * data, int length){
    OnvifMgrSerializable * ret = NULL;
    
    g_return_val_if_fail (g_type_is_a (type,OMGR_TYPE_SERIALIZABLE), NULL);
    C_TRACE("Unserializing type %s",g_type_name(type));

    gpointer class_ref = g_type_class_ref(type);
    g_return_val_if_fail (class_ref != NULL,NULL);

    OnvifMgrSerializableInterface * iface = g_type_interface_peek(class_ref,OMGR_TYPE_SERIALIZABLE);
    g_return_val_if_fail (iface != NULL,NULL);
    g_return_val_if_fail (iface->unserialize != NULL,NULL);
    ret = iface->unserialize (data,length);
    if(ret) g_return_val_if_fail (OMGR_IS_SERIALIZABLE(ret),NULL);

    g_type_class_unref(class_ref);
    return ret;
}

unsigned char * OnvifMgrSerializable__serialize(OnvifMgrSerializable * self, int * serialized_length){
    OnvifMgrSerializableInterface *iface;

    g_return_val_if_fail (OMGR_IS_SERIALIZABLE (self),FALSE);
    C_TRACE("Serializing type %s",g_type_name(G_TYPE_FROM_INSTANCE(self)));
    iface = OMGR_SERIALIZABLE_GET_IFACE (self);
    g_return_val_if_fail (iface->serialize != NULL,FALSE);
    return iface->serialize (self,serialized_length);
}