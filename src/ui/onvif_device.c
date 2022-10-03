#include "onvif_device.h"
#include "../soap/client.h"
#include "../generated/soapH.h"
#include "../generated/DeviceBinding.nsmap"

OnvifCapabilities* OnvifDevice__device_getCapabilities(OnvifDevice* self) {
    struct _tds__GetCapabilities gethostname;
    struct _tds__GetCapabilitiesResponse response;

    memset (&gethostname, 0, sizeof (gethostname));
    memset (&response, 0, sizeof (response));
    OnvifCapabilities* capabilities = (OnvifCapabilities *) malloc(sizeof(OnvifCapabilities));
    
    if (soap_call___tds__GetCapabilities(self->device_soap->soap, self->device_soap->endpoint, "", &gethostname,  &response) == SOAP_OK){
        OnvifMedia* media =  (OnvifMedia *) malloc(sizeof(OnvifMedia));
        media->xaddr = response.Capabilities->Media->XAddr;
        capabilities->media = media;
    } else {
        soap_print_fault(self->device_soap->soap, stderr);
    }

    printf("return capabilities\n");
    return capabilities; 
}

OnvifSoapClient * OnvifDevice__device_getMediaSoap(OnvifDevice* self){
    OnvifCapabilities* capabilities = OnvifDevice__device_getCapabilities(self);
    return OnvifSoapClient__create(capabilities->media->xaddr);
}

char * OnvifDevice__device_getHostname(OnvifDevice* self) {
    struct _tds__GetHostname gethostname;
    struct _tds__GetHostnameResponse response;
    
    if (soap_call___tds__GetHostname(self->device_soap->soap, self->device_soap->endpoint, "", &gethostname,  &response) == SOAP_OK){
        return response.HostnameInformation->Name;
    } else {
        soap_print_fault(self->device_soap->soap, stderr);
    }
    //TODO error handling timout, invalid url, etc...
    return "error";
}

void * OnvifDevice__media_getStreamUri(OnvifDevice* self){
    struct _trt__GetStreamUri req; //trt__GetStreamUri
    struct _trt__GetStreamUriResponse resp; //trt__GetStreamUriResponse
    memset (&req, 0, sizeof (req));
    memset (&resp, 0, sizeof (resp));
    
    if (soap_call___trt__GetStreamUri(self->media_soap->soap, self->media_soap->endpoint, "", &req, &resp) == SOAP_OK){
        return resp.MediaUri->Uri;
    } else {
        soap_print_fault(self->device_soap->soap, stderr);
    }
    //TODO error handling timout, invalid url, etc...
    return "error";
}

void OnvifDevice__init(OnvifDevice* self, char * device_url) {
    self->device_soap = OnvifSoapClient__create(device_url);
    self->hostname = OnvifDevice__device_getHostname(self);

    self->media_soap = OnvifDevice__device_getMediaSoap(self);
}

OnvifDevice OnvifDevice__create(char * device_url) {
//    OnvifDevice* result = (OnvifDevice*) malloc(sizeof(OnvifDevice));
    OnvifDevice result;
    memset (&result, 0, sizeof (result));
    OnvifDevice__init(&result,device_url);
    return result;
}


void OnvifDevice__reset(OnvifDevice* self) {
}

void OnvifDevice__destroy(OnvifDevice* device) {
  if (device) {
     OnvifDevice__reset(device);
     free(device);
  }
}