#ifndef ADD_DEV_H_ 
#define ADD_DEV_H_

#include <gtk/gtk.h>
#include "dialog/app_dialog.h"

typedef struct _AddDeviceDialog AddDeviceDialog;

typedef struct _AddDeviceDialog {
    AppDialog parent;
    void * elements;
} AddDeviceDialog;

AddDeviceDialog * AddDeviceDialog__create();
const char * AddDeviceDialog__get_device_uri(AddDeviceDialog * dialog);

#endif