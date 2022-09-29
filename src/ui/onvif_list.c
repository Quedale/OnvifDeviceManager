
#include <stddef.h>
#include <stdio.h>
#include "../soap/client.h"
#include "onvif_list.h"

void OnvifDeviceList__init(OnvifDeviceList* self) {
    self->devices=malloc(0);
    self->device_count=0;
}

OnvifDeviceList* OnvifDeviceList__create() {
   OnvifDeviceList* result = (OnvifDeviceList*) malloc(sizeof(OnvifDeviceList));
   OnvifDeviceList__init(result);
   return result;
}


void OnvifDeviceList__reset(OnvifDeviceList* self) {
}

void OnvifDeviceList__destroy(OnvifDeviceList* OnvifDeviceList) {
  if (OnvifDeviceList) {
     OnvifDeviceList__reset(OnvifDeviceList);
     free(OnvifDeviceList);
  }
}

OnvifDevice * OnvifDeviceList__remove_element_and_shift(OnvifDeviceList* self, OnvifDevice *array, int index, int array_length)
{
    int i;
    for(i = index; i < array_length; i++) {
        array[i] = array[i + 1];
    }
    return array;
};

void * OnvifDeviceList__insert_element(OnvifDeviceList* self, OnvifDevice record, int index)
{ 
    int i;
    int count = self->device_count;
    self->devices = (OnvifDevice *) realloc (self->devices,sizeof (OnvifDevice) * (count+1));
    for(i=self->device_count; i> index; i--){
        self->devices[i] = self->devices[i-1];
    }
    self->devices[index]=record;
    self->device_count++;
};

void * OnvifDeviceList__clear(OnvifDeviceList* self){
    self->device_count = 0;
    self->devices = realloc(self->devices,0);
}

void * OnvifDeviceList__remove_element(OnvifDeviceList* self, int index){
    //Remove element and shift content
    self->devices = OnvifDeviceList__remove_element_and_shift(self,self->devices, index, self->device_count);  /* First shift the elements, then reallocate */
    //Resize count
    self->device_count--;
    //Assign arythmatic
    int count = self->device_count; 
    //Resize array memory
    self->devices = realloc (self->devices,sizeof(OnvifDevice) * count);
};