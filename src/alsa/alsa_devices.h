#ifndef ALSA_DEVICES_H_ 
#define ALSA_DEVICES_H_

typedef struct {
    int card_index;
    char * card_id;
    char * card_name;
    int dev_index;
    char * dev_id;
    char * dev_name;
} AlsaDevice;

typedef struct {
  int count;
  AlsaDevice ** devices;
} AlsaDevices;

AlsaDevices* AlsaDevices__create(); 

void AlsaDevices__destroy(AlsaDevices* devices);

void AlsaDevices__insert_element(AlsaDevices* self, AlsaDevice * record, int index);

void AlsaDevices__remove_element(AlsaDevices* self,  int index);
 
void AlsaDevices__clear(AlsaDevices* self);

AlsaDevice* AlsaDevice__create(); 

void AlsaDevice__destroy(AlsaDevice* device);

void AlsaDevice__set_card_index(AlsaDevice * self, int card_index);

void AlsaDevice__set_card_id(AlsaDevice * self, char * card_id);

void AlsaDevice__set_card_name(AlsaDevice * self, char * card_name);

void AlsaDevice__set_dev_index(AlsaDevice * self, int dev_index);

void AlsaDevice__set_dev_id(AlsaDevice * self, char * dev_id);

void AlsaDevice__set_dev_name(AlsaDevice * self, char * dev_name);

#endif