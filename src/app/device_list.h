#ifndef DEVICE_LIST_H_ 
#define DEVICE_LIST_H_

#include "onvif_device.h"
#include <gtk/gtk.h>
#include "../queue/event_queue.h"

typedef struct _Device Device;

typedef struct _Device {
  OnvifDevice * onvif_device;
  GtkWidget * image_handle;
  GtkWidget * profile_dropdown;
  pthread_mutex_t * ref_lock;
  int profile_index;
  int destroyed;
  int refcount;
  int selected;

  void (*profile_callback)(Device *, void *);
  void * profile_userdata;
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
Device * Device__create(OnvifDevice * onvif_device);
int Device__addref(Device* device); 
void Device__unref(Device* device); 
int Device__is_valid(Device* device); 
void Device__set_profile_callback(Device * device, void (*profile_callback)(Device *, void *), void * profile_userdata);
void Device__lookup_hostname(Device* device, EventQueue * queue); 
void Device__load_thumbnail(Device* device, EventQueue * queue);
void Device__load_profiles(Device* device, EventQueue * queue);
GtkWidget * Device__create_row (Device * device, char * uri, char* name, char * hardware, char * location);

#endif