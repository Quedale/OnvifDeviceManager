#include "../generated/soapH.h"
#include "../generated/DeviceBinding.nsmap"
#include "client.h"

void OnvifSoapClient__init(OnvifSoapClient* self, char * endpoint) {
    self->endpoint=endpoint;
    self->soap= soap_new();
}

OnvifSoapClient* OnvifSoapClient__create(char * endpoint) {
   OnvifSoapClient* result = (OnvifSoapClient*) malloc(sizeof(OnvifSoapClient));
   OnvifSoapClient__init(result, endpoint);
   return result;
}

void OnvifSoapClient__reset(OnvifSoapClient* self) {
}

void OnvifSoapClient__destroy(OnvifSoapClient* OnvifSoapClient) {
  if (OnvifSoapClient) {
     OnvifSoapClient__reset(OnvifSoapClient);
     free(OnvifSoapClient);
  }
}

char * OnvifSoapClient__getHostname(OnvifSoapClient* self) {
    struct _tds__GetHostname gethostname;
    struct _tds__GetHostnameResponse response;
    
    if (soap_call___tds__GetHostname(self->soap, self->endpoint, "", &gethostname,  &response) == SOAP_OK){
        return response.HostnameInformation->Name;
    } else {
        soap_print_fault(self->soap, stderr);
    }
    //TODO error handling timout, invalid url, etc...
    return "error";
}

// int main(){
//     OnvifSoapClient* p = OnvifSoapClient__create("http://192.168.1.15:8081/onvif/device_service");

//     printf("endpoint : %s\n",p->endpoint);
//     printf("endpoint : %s\n",OnvifSoapClient__getHostname(p));
// }