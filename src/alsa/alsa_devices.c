#include "alsa_devices.h"
#include "clogger.h"
#include "cobject.h"
#include <stdlib.h>





void priv_AlsaDevice__destroy(CObject * self){
    AlsaDevice * device = (AlsaDevice*) self;
    if(device){
        if(device->card_id)
            free(device->card_id);
        if(device->card_name)
            free(device->card_name);
        if(device->dev_id)
            free(device->dev_id);
        if(device->dev_name)
            free(device->dev_name);
    }
}

void AlsaDevice__init(AlsaDevice* self) {
    CObject__init((CObject*)self);
    CObject__set_destroy_callback((CObject*)self,priv_AlsaDevice__destroy);
    self->card_index = -1;
    self->card_id = NULL;
    self->card_name = NULL;
    self->dev_index = -1;
    self->dev_id = NULL;
    self->dev_name = NULL;
}


AlsaDevice* AlsaDevice__create() {
    C_DEBUG("Create AslaDevice object");
    AlsaDevice* result = (AlsaDevice*) malloc(sizeof(AlsaDevice));
    AlsaDevice__init(result);
    CObject__set_allocated((CObject*)result);
    return result;
}

void AlsaDevice__set_card_index(AlsaDevice * self, int card_index){
    self->card_index = card_index;
}

void AlsaDevice__set_card_id(AlsaDevice * self, char * card_id){
    if(self->card_id){
        self->card_id = realloc(self->card_id,strlen(card_id)+1);
    } else {
        self->card_id = malloc(strlen(card_id)+1);
    }
    strcpy(self->card_id,card_id);
}

void AlsaDevice__set_card_name(AlsaDevice * self, char * card_name){
    if(self->card_name){
        self->card_name = realloc(self->card_name,strlen(card_name)+1);
    } else {
        self->card_name = malloc(strlen(card_name)+1);
    }

    strcpy(self->card_name,card_name);
}

void AlsaDevice__set_dev_index(AlsaDevice * self, int dev_index){
    self->dev_index = dev_index;
}

void AlsaDevice__set_dev_id(AlsaDevice * self, char * dev_id){
    if(self->dev_id){
        self->dev_id = realloc(self->dev_id,strlen(dev_id)+1);
    } else {
        self->dev_id = malloc(strlen(dev_id)+1);
    }
    strcpy(self->dev_id,dev_id);
}

void AlsaDevice__set_dev_name(AlsaDevice * self, char * dev_name){
    if(self->dev_name){
        self->dev_name = realloc(self->dev_name,strlen(dev_name)+1);
    } else {
        self->dev_name = malloc(strlen(dev_name)+1);
    }
    strcpy(self->dev_name,dev_name);
}
