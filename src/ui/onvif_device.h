#ifndef ONVIF_DEVICE_H_ 
#define ONVIF_DEVICE_H_

#include "../soap/client.h"

typedef struct {
    char *xaddr;
    //TODO StreamingCapabilities
    //TODO Extension
} OnvifMedia;

typedef struct {
    OnvifMedia *media;
} OnvifCapabilities;

typedef struct {
    char * manufacturer;
    char * model;
    char * firmwareVersion;
    char * serialNumber;
    char * hardwareId;
} OnvifDeviceInformation;

typedef struct {
    char * hostname;
    int authorized;
    OnvifSoapClient* device_soap;
    OnvifSoapClient* media_soap;
    // OnvifCapabilities * capabilities;
} OnvifDevice;

OnvifDevice OnvifDevice__create(char * device_url); 
void OnvifDevice__destroy(OnvifDevice* device); 
// char * OnvifDevice__device_getHostname(OnvifDevice* self);  // equivalent to "point->x()"
OnvifCapabilities* OnvifDevice__device_getCapabilities(OnvifDevice* self);
OnvifDeviceInformation * OnvifDevice__device_getDeviceInformation(OnvifDevice *self);
void * OnvifDevice__media_getStreamUri(OnvifDevice* self);

#endif