
#include <stddef.h>
#include <stdio.h>
#include "client.h"
#include "device_list.h"

void DeviceList__init(DeviceList* self) {
    self->devices=malloc(0);
    self->device_count=0;
}

DeviceList* DeviceList__create() {
   DeviceList* result = (DeviceList*) malloc(sizeof(DeviceList));
   DeviceList__init(result);
   return result;
}



void DeviceList__destroy(DeviceList* self) {
  if (self) {
     DeviceList__clear(self);
     free(self->devices);
     free(self);
  }
}

Device ** DeviceList__remove_element_and_shift(DeviceList* self, Device **array, int index, int array_length)
{
    int i;
    for(i = index; i < array_length; i++) {
        array[i] = array[i + 1];
    }
    return array;
};

void DeviceList__insert_element(DeviceList* self, Device * record, int index)
{ 
    int i;
    int count = self->device_count;
    self->devices = realloc (self->devices,sizeof (Device) * (count+1));
    for(i=self->device_count; i> index; i--){
        self->devices[i] = self->devices[i-1];
    }
    self->devices[index]=record;
    self->device_count++;
};

void DeviceList__clear(DeviceList* self){
    int i;
    for(i=0; i < self->device_count; i++){
        Device__destroy(self->devices[i]);
    }
    self->device_count = 0;
    self->devices = realloc(self->devices,0);
}

void DeviceList__remove_element(DeviceList* self, int index){
    //Remove element and shift content
    self->devices = DeviceList__remove_element_and_shift(self,self->devices, index, self->device_count);  /* First shift the elements, then reallocate */
    //Resize count
    self->device_count--;
    //Assign arythmatic
    int count = self->device_count; 
    //Resize array memory
    self->devices = realloc (self->devices,sizeof(Device) * count);
};


void Device__init(Device* self, OnvifDevice * onvif_device) {
    self->onvif_device = onvif_device;
}

Device * Device_create(OnvifDevice * onvif_device){
    Device* result = (Device*) malloc(sizeof(Device));
    Device__init(result, onvif_device);
    return result;
}

void Device__destroy(Device* self) {
  if (self) {
     OnvifDevice__destroy(self->onvif_device);
     free(self);
  }
}