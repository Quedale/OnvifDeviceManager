#ifndef ALSA_UTILS_H_ 
#define ALSA_UTILS_H_

#include "portable_thread.h"
PUSH_WARNING_IGNORE(-1,-Wpedantic)
#include <alsa/asoundlib.h>
POP_WARNING_IGNORE(NULL)
#include "alsa_devices.h"
#include "clist_ts.h"

CListTS * get_alsa_device_list(snd_pcm_stream_t stream);

#endif