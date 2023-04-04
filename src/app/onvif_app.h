
#ifndef ONVIF_APP_H_ 
#define ONVIF_APP_H_

typedef struct _OnvifApp OnvifApp;

OnvifApp * OnvifApp__create();
void OnvifApp__destroy(OnvifApp* self);

#endif