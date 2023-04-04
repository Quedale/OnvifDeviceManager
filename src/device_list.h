#ifndef DEVICE_LIST_H_ 
#define DEVICE_LIST_H_

#include "onvif_device.h"
#include <gtk/gtk.h>

typedef struct {
  OnvifDevice * onvif_device;
  GtkWidget * image_handle;
  GtkWidget * profile_dropdown;
  pthread_mutex_t * ref_lock;
  int destroyed;
  int refcount;
  int selected;
} Device;

typedef struct {
  int device_count;
  Device **devices;
} DeviceList;

DeviceList* DeviceList__create(); 

void DeviceList__destroy(DeviceList* onvifDeviceList);
void DeviceList__insert_element(DeviceList* self, Device * record, int index);
void DeviceList__remove_element(DeviceList* self, int index);
void DeviceList__clear(DeviceList* self);

void Device__destroy(Device* self);
Device * Device_create(OnvifDevice * onvif_device);
int Device__addref(Device* device); 
void Device__unref(Device* device); 
int Device__is_valid(Device* device); 

#endif