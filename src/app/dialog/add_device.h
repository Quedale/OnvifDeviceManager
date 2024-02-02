#ifndef ADD_DEV_H_ 
#define ADD_DEV_H_

#include <gtk/gtk.h>
#include "app_dialog.h"

typedef struct _AddDeviceDialog AddDeviceDialog;

typedef struct _AddDeviceDialog {
    AppDialog parent;
    void * elements;
} AddDeviceDialog;

AddDeviceDialog * AddDeviceDialog__create();
const char * AddDeviceDialog__get_host(AddDeviceDialog * dialog);
const char * AddDeviceDialog__get_port(AddDeviceDialog * dialog);
const char * AddDeviceDialog__get_user(AddDeviceDialog * dialog);
const char * AddDeviceDialog__get_pass(AddDeviceDialog * dialog);
const char * AddDeviceDialog__get_protocol(AddDeviceDialog * dialog);
void AddDeviceDialog__set_error(AddDeviceDialog * self, char * error);

#endif