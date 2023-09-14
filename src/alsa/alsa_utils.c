
#include <alsa/asoundlib.h>
#include "alsa_utils.h"
#include "alsa_devices.h"
#include <gst/gstinfo.h>

GST_DEBUG_CATEGORY_STATIC (ext_alsautils_debug);
#define GST_CAT_DEFAULT (ext_alsautils_debug)

AlsaDevices* get_alsa_device_list(snd_pcm_stream_t stream){
    GST_DEBUG_CATEGORY_INIT (ext_alsautils_debug, "ext-alsa-utils", 0, "Extension to support Alsa capabilities");

    AlsaDevices* list = AlsaDevices__create(); 

	snd_ctl_t *handle;
	int card, err, dev; //, idx;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	card = -1;
	if (snd_card_next(&card) < 0 || card < 0) {
		GST_ERROR("no soundcards found...\n");
		return list;
	}
	GST_DEBUG("**** List of Hardware Devices %s****\n", snd_pcm_stream_name(stream));
	while (card >= 0) {
		char name[32];
		sprintf(name, "hw:%d", card);
		if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
			GST_ERROR("control open (%i): %s\n", card, snd_strerror(err));
			goto next_card;
		}
		if ((err = snd_ctl_card_info(handle, info)) < 0) {
			GST_ERROR("control hardware info (%i): %s\n", card, snd_strerror(err));
			snd_ctl_close(handle);
			goto next_card;
		}
		dev = -1;
		while (1) {
			// unsigned int count;
			if (snd_ctl_pcm_next_device(handle, &dev)<0)
				GST_ERROR("snd_ctl_pcm_next_device\n");
			if (dev < 0)
				break;
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
			if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) {
				if (err != -ENOENT)
					GST_ERROR("control digital audio info (%i): %s\n", card, snd_strerror(err));
				continue;
			}
			GST_DEBUG("card %i: %s [%s], device %i: %s [%s]\n",
				card, snd_ctl_card_info_get_id(info), snd_ctl_card_info_get_name(info),
				dev,
				snd_pcm_info_get_id(pcminfo),
				snd_pcm_info_get_name(pcminfo));

            AlsaDevice * alsadev = AlsaDevice__create();
            AlsaDevice__set_card_index(alsadev,card);
            AlsaDevice__set_card_id(alsadev,(char *) snd_ctl_card_info_get_id(info));
            AlsaDevice__set_card_name(alsadev,(char *) snd_ctl_card_info_get_name(info));
            AlsaDevice__set_dev_index(alsadev,dev);
            AlsaDevice__set_dev_id(alsadev,(char *) snd_pcm_info_get_id(pcminfo));
            AlsaDevice__set_dev_name(alsadev,(char *) snd_pcm_info_get_name(pcminfo));
            AlsaDevices__insert_element(list,alsadev,list->count);
		}
		snd_ctl_close(handle);
	next_card:
		if (snd_card_next(&card) < 0) {
			GST_ERROR("snd_card_next\n");
			break;
		}
	}

    return list;
}
