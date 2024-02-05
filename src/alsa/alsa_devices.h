#ifndef ALSA_DEVICES_H_ 
#define ALSA_DEVICES_H_

#include "cobject.h"

typedef struct {
  CObject parent;
  int card_index;
  char * card_id;
  char * card_name;
  int dev_index;
  char * dev_id;
  char * dev_name;
} AlsaDevice;

AlsaDevice* AlsaDevice__create(); 

void AlsaDevice__set_card_index(AlsaDevice * self, int card_index);

void AlsaDevice__set_card_id(AlsaDevice * self, char * card_id);

void AlsaDevice__set_card_name(AlsaDevice * self, char * card_name);

void AlsaDevice__set_dev_index(AlsaDevice * self, int dev_index);

void AlsaDevice__set_dev_id(AlsaDevice * self, char * dev_id);

void AlsaDevice__set_dev_name(AlsaDevice * self, char * dev_name);

#endif