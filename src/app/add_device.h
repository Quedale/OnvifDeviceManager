#ifndef ADD_DEV_H_ 
#define ADD_DEV_H_

#include <gtk/gtk.h>

typedef struct {
    int visible;
    GtkWidget * root;
    void (*add_callback)();
    void (*cancel_callback)();
    void * user_data;
    void * private;
} AddDeviceDialog;

typedef struct { 
    char * device_url;
    AddDeviceDialog * dialog;
    void * user_data;
} AddDeviceEvent;

AddDeviceDialog * AddDeviceDialog__create();
void AddDeviceDialog__destroy(AddDeviceDialog* dialog);
void AddDeviceDialog__show(AddDeviceDialog* dialog, void (*add_callback)(AddDeviceEvent *), void (*cancel_callback)(AddDeviceDialog *), void * user_data);
void AddDeviceDialog__hide(AddDeviceDialog* dialog);
AddDeviceEvent * AddDeviceDialog_copy(AddDeviceEvent * original);
AddDeviceEvent * AddDeviceEvent_copy(AddDeviceEvent * original);

#endif