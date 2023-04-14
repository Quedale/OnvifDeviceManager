#include "alsa_devices.h"
#include <gst/gstinfo.h>

GST_DEBUG_CATEGORY_STATIC (ext_alsa_debug);
#define GST_CAT_DEFAULT (ext_alsa_debug)

void AlsaDevices__init(AlsaDevices* self) {
    self->devices=malloc(0);
    self->count=0;
}

AlsaDevices* AlsaDevices__create() {
    GST_DEBUG_CATEGORY_INIT (ext_alsa_debug, "ext-alsa", 0, "Extension to support Alsa capabilities");

    GST_DEBUG("Create AlsaDevices list");
    AlsaDevices* result = (AlsaDevices*) malloc(sizeof(AlsaDevices));
    AlsaDevices__init(result);
    return result;
}

void AlsaDevices__destroy(AlsaDevices* alsaDevices) {
  if (alsaDevices) {
     AlsaDevices__clear(alsaDevices);
     free(alsaDevices->devices);
     free(alsaDevices);
  }
}


void AlsaDevices__clear(AlsaDevices* self){
    self->count = 0;
    int i;
    for(i=0;i<self->count;i++){
        AlsaDevice__destroy(self->devices[i]);
    }
    self->devices = realloc(self->devices,0);
}

void AlsaDevices__remove_element_and_shift(AlsaDevices * self, int index)
{
    int i;
    for(i = index; i < self->count; i++) {
        self->devices[i] = self->devices[i + 1];
    }
    self->count--;
}

void AlsaDevices__remove_element(AlsaDevices * self, int index){
    //Remove element and shift content
    AlsaDevices__remove_element_and_shift(self, index);  /* First shift the elements, then reallocate */

    //Resize array memory
    self->devices = realloc (self->devices,sizeof(AlsaDevice) * self->count);
}

void AlsaDevices__insert_element(AlsaDevices* self, AlsaDevice * record, int index)
{   
    int i;
    self->count++;
    self->devices = realloc (self->devices,sizeof (AlsaDevice) * (self->count));
    for(i=self->count; i> index; i--){
        self->devices[i] = self->devices[i-1];
    }
    self->devices[index]=record;
}

void AlsaDevice__init(AlsaDevice* self) {
    self->card_index = -1;
    self->card_id = malloc(0);
    self->card_name = malloc(0);
    self->dev_index = -1;
    self->dev_id = malloc(0);
    self->dev_name = malloc(0);
}


AlsaDevice* AlsaDevice__create() {
    GST_DEBUG_CATEGORY_INIT (ext_alsa_debug, "ext-alsa", 0, "Extension to support Alsa capabilities");

    GST_DEBUG("Create AslaDevice object");
    AlsaDevice* result = (AlsaDevice*) malloc(sizeof(AlsaDevice));
    AlsaDevice__init(result);
    return result;
}

void AlsaDevice__set_card_index(AlsaDevice * self, int card_index){
    self->card_index = card_index;
}

void AlsaDevice__set_card_id(AlsaDevice * self, char * card_id){
    self->card_id = realloc(self->card_id,strlen(card_id)+1);
    strcpy(self->card_id,card_id);
}

void AlsaDevice__set_card_name(AlsaDevice * self, char * card_name){
    self->card_name = realloc(self->card_name,strlen(card_name)+1);
    strcpy(self->card_name,card_name);
}

void AlsaDevice__set_dev_index(AlsaDevice * self, int dev_index){
    self->dev_index = dev_index;
}

void AlsaDevice__set_dev_id(AlsaDevice * self, char * dev_id){
    self->dev_id = realloc(self->dev_id,strlen(dev_id)+1);
    strcpy(self->dev_id,dev_id);
}

void AlsaDevice__set_dev_name(AlsaDevice * self, char * dev_name){
    self->dev_name = realloc(self->dev_name,strlen(dev_name)+1);
    strcpy(self->dev_name,dev_name);
}

void AlsaDevice__destroy(AlsaDevice* device){
    free(device->card_id);
    free(device->card_name);
    free(device->dev_id);
    free(device->dev_name);
    free(device);
}