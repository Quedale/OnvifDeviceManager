#include "../generated/soapH.h"
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