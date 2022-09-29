#include "generated/soapH.h"
#include "generated/DeviceBinding.nsmap"

int main()
{
   struct soap *soap = soap_new();
   struct _tds__GetHostname gethostname;
   struct _tds__GetHostnameResponse response;
   
   if (soap_call___tds__GetHostname(soap, "http://192.168.1.15:8081/onvif/device_service", "", &gethostname,  &response) == SOAP_OK){
      printf("response : %s\n", response.HostnameInformation->Name);
   } else {
      soap_print_fault(soap, stderr);
   }
   soap_end(soap);
   soap_free(soap);
} 