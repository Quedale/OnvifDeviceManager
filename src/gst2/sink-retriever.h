

typedef enum {
  ONVIF_NA               = 0,
  ONVIF_PULSE             = 1,
  ONVIF_ASLA             = 2,
  ONVIF_OMX               =3
  //TODO Handle more types
} SupportedAudioSinkTypes;

SupportedAudioSinkTypes 
retrieve_audiosink (void);