#include "onvif_device.h"
#include "../soap/client.h"
#include "../generated/soapH.h"
#include "../generated/DeviceBinding.nsmap"
#include "wsseapi.h"

OnvifCapabilities* OnvifDevice__device_getCapabilities(OnvifDevice* self) {
    struct _tds__GetCapabilities gethostname;
    struct _tds__GetCapabilitiesResponse response;

    memset (&gethostname, 0, sizeof (gethostname));
    memset (&response, 0, sizeof (response));
    OnvifCapabilities* capabilities = (OnvifCapabilities *) malloc(sizeof(OnvifCapabilities));
    

    if (soap_wsse_add_Timestamp(self->device_soap->soap, "Time", 10)
        || soap_wsse_add_UsernameTokenDigest(self->device_soap->soap, "Auth", "admin", "admin")){
        printf("Unable to set wsse creds...\n");
        //TODO Error handling
        return "Error";
    }

    if (soap_call___tds__GetCapabilities(self->device_soap->soap, self->device_soap->endpoint, "", &gethostname,  &response) == SOAP_OK){
        OnvifMedia* media =  (OnvifMedia *) malloc(sizeof(OnvifMedia));
        media->xaddr = response.Capabilities->Media->XAddr;
        capabilities->media = media;
    } else {
        soap_print_fault(self->device_soap->soap, stderr);
    }

    return capabilities; 
}

OnvifSoapClient * OnvifDevice__device_getMediaSoap(OnvifDevice* self){
    OnvifCapabilities* capabilities = OnvifDevice__device_getCapabilities(self);
    return OnvifSoapClient__create(capabilities->media->xaddr);
}

//GetGeoLocation
//
OnvifDeviceInformation * OnvifDevice__device_getDeviceInformation(OnvifDevice *self){
    struct _tds__GetDeviceInformation request;
    struct _tds__GetDeviceInformationResponse response;
    OnvifDeviceInformation *ret = (OnvifDeviceInformation *) malloc(sizeof(OnvifDeviceInformation));

    if (soap_wsse_add_Timestamp(self->device_soap->soap, "Time", 10)
        || soap_wsse_add_UsernameTokenDigest(self->device_soap->soap, "Auth", "admin", "admin")){
        printf("Unable to set wsse creds...\n");
        //TODO Error handling
        return "Error";
    }

    if (soap_call___tds__GetDeviceInformation(self->device_soap->soap, self->device_soap->endpoint, "", &request,  &response) == SOAP_OK){
        ret->firmwareVersion = response.FirmwareVersion;
        ret->hardwareId = response.HardwareId;
        ret->manufacturer = response.Manufacturer;
        ret->model = response.Model;
        ret->serialNumber = response.SerialNumber;
        return ret;
    } else {
        soap_print_fault(self->device_soap->soap, stderr);
        //TODO error handling timout, invalid url, etc...
        return NULL;
    }
}

char * OnvifDevice__device_getHostname(OnvifDevice* self) {
    struct _tds__GetHostname gethostname;
    struct _tds__GetHostnameResponse response;

    if (soap_wsse_add_Timestamp(self->device_soap->soap, "Time", 10)
        || soap_wsse_add_UsernameTokenDigest(self->device_soap->soap, "Auth", "admin", "admin")){
        printf("Unable to set wsse creds...\n");
        //TODO Error handling
        return "Error";
    }

    if (soap_call___tds__GetHostname(self->device_soap->soap, self->device_soap->endpoint, "", &gethostname,  &response) == SOAP_OK){
        return response.HostnameInformation->Name;
    } else {
        soap_print_fault(self->device_soap->soap, stderr);
    }
    //TODO error handling timout, invalid url, etc...
    return "error";
}

void * OnvifDevice__media_getStreamUri(OnvifDevice* self){
    struct _trt__GetStreamUri req;
    struct _trt__GetStreamUriResponse resp;
    memset (&req, 0, sizeof (req));
    memset (&resp, 0, sizeof (resp));

    if (soap_wsse_add_Timestamp(self->media_soap->soap, "Time", 10)
        || soap_wsse_add_UsernameTokenDigest(self->media_soap->soap, "Auth", "admin", "admin")){
        printf("Unable to set wsse creds...\n");
        //TODO Error handling
        return "Error";
    }

    if (soap_call___trt__GetStreamUri(self->media_soap->soap, self->media_soap->endpoint, "", &req, &resp) == SOAP_OK){
        return resp.MediaUri->Uri;
    } else {
        soap_print_fault(self->media_soap->soap, stderr);
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