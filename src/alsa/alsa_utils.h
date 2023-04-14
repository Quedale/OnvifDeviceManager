#ifndef ALSA_UTILS_H_ 
#define ALSA_UTILS_H_

#include <alsa/asoundlib.h>
#include "alsa_devices.h"

AlsaDevices* get_alsa_device_list(snd_pcm_stream_t stream);

#endif