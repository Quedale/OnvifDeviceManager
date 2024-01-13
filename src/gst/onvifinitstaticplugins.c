
#include <gst/gst.h>
#include <stdio.h>
#include "clogger.h"

GST_PLUGIN_STATIC_DECLARE(gtknew);

#ifdef STATIC_BUILD



GST_PLUGIN_STATIC_DECLARE(coreelements);
// GST_PLUGIN_STATIC_DECLARE(coretracers);
// GST_PLUGIN_STATIC_DECLARE(adder); gstadder
GST_PLUGIN_STATIC_DECLARE(app);
GST_PLUGIN_STATIC_DECLARE(audioconvert);
// GST_PLUGIN_STATIC_DECLARE(audiomixer); gstaudiomixer
// GST_PLUGIN_STATIC_DECLARE(audiorate); gstaudiorate
GST_PLUGIN_STATIC_DECLARE(audioresample);
GST_PLUGIN_STATIC_DECLARE(audiotestsrc);
// GST_PLUGIN_STATIC_DECLARE(compositor); gstcompositor
// GST_PLUGIN_STATIC_DECLARE(encoding); gstencoding
// GST_PLUGIN_STATIC_DECLARE(gio); gstgio
GST_PLUGIN_STATIC_DECLARE(overlaycomposition);
GST_PLUGIN_STATIC_DECLARE(pbtypes);
GST_PLUGIN_STATIC_DECLARE(playback);
GST_PLUGIN_STATIC_DECLARE(rawparse);
// GST_PLUGIN_STATIC_DECLARE(subparse); gstsubparse
GST_PLUGIN_STATIC_DECLARE(tcp);
GST_PLUGIN_STATIC_DECLARE(typefindfunctions);
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
// GST_PLUGIN_STATIC_DECLARE(videorate); gstvideorate
GST_PLUGIN_STATIC_DECLARE(videotestsrc);
GST_PLUGIN_STATIC_DECLARE(volume); // gstvolume
GST_PLUGIN_STATIC_DECLARE(alsa);
// GST_PLUGIN_STATIC_DECLARE(opengl); gstopengl
// GST_PLUGIN_STATIC_DECLARE(ogg); gstogg
// GST_PLUGIN_STATIC_DECLARE(opus); gstopus
// GST_PLUGIN_STATIC_DECLARE(vorbis); gstvorbis
GST_PLUGIN_STATIC_DECLARE(ximagesink);
// GST_PLUGIN_STATIC_DECLARE(xvimagesink);
// GST_PLUGIN_STATIC_DECLARE(alpha); gstalpha
// GST_PLUGIN_STATIC_DECLARE(alphacolor); gstalphacolor
// GST_PLUGIN_STATIC_DECLARE(apetag); gstapetag
// GST_PLUGIN_STATIC_DECLARE(audiofx); gstaudiofx
GST_PLUGIN_STATIC_DECLARE(audioparsers);
// GST_PLUGIN_STATIC_DECLARE(auparse); gstauparse
GST_PLUGIN_STATIC_DECLARE(autodetect);
// GST_PLUGIN_STATIC_DECLARE(avi); gstavi
// GST_PLUGIN_STATIC_DECLARE(cutter); gstcutter
// GST_PLUGIN_STATIC_DECLARE(navigationtest); gstnavigationtest
// GST_PLUGIN_STATIC_DECLARE(debug); gstdebug
// GST_PLUGIN_STATIC_DECLARE(deinterlace); gstdeinterlace
// GST_PLUGIN_STATIC_DECLARE(dtmf); gstdtmf
// GST_PLUGIN_STATIC_DECLARE(effectv); gsteffectv
// GST_PLUGIN_STATIC_DECLARE(equalizer); gstequalizer
// GST_PLUGIN_STATIC_DECLARE(flv); gstflv
// GST_PLUGIN_STATIC_DECLARE(flxdec); gstflxdec
// GST_PLUGIN_STATIC_DECLARE(goom); gstgoom
// GST_PLUGIN_STATIC_DECLARE(goom2k1); gstgoom2k1
// GST_PLUGIN_STATIC_DECLARE(icydemux); gsticydemux
// GST_PLUGIN_STATIC_DECLARE(id3demux); gstid3demux
// GST_PLUGIN_STATIC_DECLARE(imagefreeze); gstimagefreeze
GST_PLUGIN_STATIC_DECLARE(interleave); // gstinterleave
// GST_PLUGIN_STATIC_DECLARE(isomp4); gstisomp4
// GST_PLUGIN_STATIC_DECLARE(alaw); gstalaw
GST_PLUGIN_STATIC_DECLARE(mulaw);
GST_PLUGIN_STATIC_DECLARE(level); // gstlevel
// GST_PLUGIN_STATIC_DECLARE(matroska); gstmatroska
// GST_PLUGIN_STATIC_DECLARE(monoscope); gstmonoscope 
// GST_PLUGIN_STATIC_DECLARE(multifile); gstmultifile
// GST_PLUGIN_STATIC_DECLARE(multipart); gstmultipart
#ifdef ENABLERPI
GST_PLUGIN_STATIC_DECLARE(omx);
GST_PLUGIN_STATIC_DECLARE(rpicamsrc); 
#endif
#ifdef ENABLELIBAV
GST_PLUGIN_STATIC_DECLARE(libav);
#endif
// GST_PLUGIN_STATIC_DECLARE(replaygain); gstreplaygain
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(rtsp);
// GST_PLUGIN_STATIC_DECLARE(shapewipe); gstshapewipe
// GST_PLUGIN_STATIC_DECLARE(smpte); gstsmpte
// GST_PLUGIN_STATIC_DECLARE(spectrum); gstspectrum
GST_PLUGIN_STATIC_DECLARE(udp);
// GST_PLUGIN_STATIC_DECLARE(videobox); gstvideobox
// GST_PLUGIN_STATIC_DECLARE(videocrop); gstvideocrop
// GST_PLUGIN_STATIC_DECLARE(videofilter); gstvideofilter
// GST_PLUGIN_STATIC_DECLARE(videomixer); gstvideomixer
// GST_PLUGIN_STATIC_DECLARE(wavenc); gstwavenc
// GST_PLUGIN_STATIC_DECLARE(wavparse); gstwavparse
// GST_PLUGIN_STATIC_DECLARE(xingmux); gstxingmux
// GST_PLUGIN_STATIC_DECLARE(y4menc); gsty4menc
// GST_PLUGIN_STATIC_DECLARE(ossaudio);
// GST_PLUGIN_STATIC_DECLARE(oss4);
GST_PLUGIN_STATIC_DECLARE(video4linux2);
// GST_PLUGIN_STATIC_DECLARE(ximagesrc); gstximagesrc
// GST_PLUGIN_STATIC_DECLARE(adaptivedemux2); gstadaptivedemux2
// GST_PLUGIN_STATIC_DECLARE(cairo);
// GST_PLUGIN_STATIC_DECLARE(gdkpixbuf); // gstgdkpixbuf
// GST_PLUGIN_STATIC_DECLARE(gtk); // gstgtk
GST_PLUGIN_STATIC_DECLARE(jpeg); // gstjpeg
// GST_PLUGIN_STATIC_DECLARE(lame); gstlame 
// GST_PLUGIN_STATIC_DECLARE(dv); gstdv
// GST_PLUGIN_STATIC_DECLARE(png); gstpng
// GST_PLUGIN_STATIC_DECLARE(soup); gstsoup
// GST_PLUGIN_STATIC_DECLARE(nice); gstnice
// GST_PLUGIN_STATIC_DECLARE(accurip); gstaccurip
// GST_PLUGIN_STATIC_DECLARE(adpcmdec); gstadpcmdec
// GST_PLUGIN_STATIC_DECLARE(adpcmenc); gstadpcmenc
// GST_PLUGIN_STATIC_DECLARE(aiff);
GST_PLUGIN_STATIC_DECLARE(asfmux);
// GST_PLUGIN_STATIC_DECLARE(audiobuffersplit); gstaudiobuffersplit
// GST_PLUGIN_STATIC_DECLARE(audiofxbad); gstaudiofxbad
// GST_PLUGIN_STATIC_DECLARE(audiomixmatrix); gstaudiomixmatrix
// GST_PLUGIN_STATIC_DECLARE(audiolatency);
// GST_PLUGIN_STATIC_DECLARE(audiovisualizers); gstaudiovisualizers
// GST_PLUGIN_STATIC_DECLARE(autoconvert); gstautoconvert
// GST_PLUGIN_STATIC_DECLARE(bayer); gstbayer
// GST_PLUGIN_STATIC_DECLARE(camerabin); gstcamerabin
// GST_PLUGIN_STATIC_DECLARE(codecalpha); gstcodecalpha
// GST_PLUGIN_STATIC_DECLARE(codectimestamper); gstcodectimestamper
// GST_PLUGIN_STATIC_DECLARE(coloreffects); gstcoloreffects
// GST_PLUGIN_STATIC_DECLARE(debugutilsbad); gstdebugutilsbad
// GST_PLUGIN_STATIC_DECLARE(dvbsubenc); gstdvbsubenc
// GST_PLUGIN_STATIC_DECLARE(dvbsuboverlay); gstdvbsuboverlay
// GST_PLUGIN_STATIC_DECLARE(dvdspu); gstdvdspu
// GST_PLUGIN_STATIC_DECLARE(faceoverlay); gstfaceoverlay
// GST_PLUGIN_STATIC_DECLARE(festival); gstfestival
// GST_PLUGIN_STATIC_DECLARE(fieldanalysis); gstfieldanalysis
// GST_PLUGIN_STATIC_DECLARE(freeverb); gstfreeverb
// GST_PLUGIN_STATIC_DECLARE(frei0r); gstfrei0r
// GST_PLUGIN_STATIC_DECLARE(gaudieffects); gstgaudieffects
// GST_PLUGIN_STATIC_DECLARE(gdp); gstgdp
// GST_PLUGIN_STATIC_DECLARE(geometrictransform); gstgeometrictransform
// GST_PLUGIN_STATIC_DECLARE(id3tag); gstid3tag
// GST_PLUGIN_STATIC_DECLARE(inter); gstinter
// GST_PLUGIN_STATIC_DECLARE(interlace); gstinterlace
// GST_PLUGIN_STATIC_DECLARE(ivfparse); gstivfparse
// GST_PLUGIN_STATIC_DECLARE(ivtc); gstivtc
// GST_PLUGIN_STATIC_DECLARE(jp2kdecimator); gstjp2kdecimator
GST_PLUGIN_STATIC_DECLARE(jpegformat); // gstjpegformat
// GST_PLUGIN_STATIC_DECLARE(rfbsrc); gstrfbsrc
// GST_PLUGIN_STATIC_DECLARE(midi); gstmidi 
// GST_PLUGIN_STATIC_DECLARE(mpegpsdemux); gstmpegpsdemux
// GST_PLUGIN_STATIC_DECLARE(mpegpsmux); gstmpegpsmux
// GST_PLUGIN_STATIC_DECLARE(mpegtsdemux);
// GST_PLUGIN_STATIC_DECLARE(mpegtsmux);
// GST_PLUGIN_STATIC_DECLARE(mxf); gstmxf
// GST_PLUGIN_STATIC_DECLARE(netsim); gstnetsim
GST_PLUGIN_STATIC_DECLARE(rtponvif);
// GST_PLUGIN_STATIC_DECLARE(pcapparse); gstpcapparse
// GST_PLUGIN_STATIC_DECLARE(pnm); gstpnm
// GST_PLUGIN_STATIC_DECLARE(proxy); gstproxy
// GST_PLUGIN_STATIC_DECLARE(legacyrawparse);
// GST_PLUGIN_STATIC_DECLARE(removesilence); gstremovesilence
// GST_PLUGIN_STATIC_DECLARE(rist); gstrist
// GST_PLUGIN_STATIC_DECLARE(rtmp2); gstrtmp2
// GST_PLUGIN_STATIC_DECLARE(rtpmanagerbad); gstrtpmanagerbad
// GST_PLUGIN_STATIC_DECLARE(sdpelem);
// GST_PLUGIN_STATIC_DECLARE(segmentclip); gstsegmentclip
// GST_PLUGIN_STATIC_DECLARE(siren); gstsiren
// GST_PLUGIN_STATIC_DECLARE(smooth); gstsmooth
// GST_PLUGIN_STATIC_DECLARE(speed); gstspeed
// GST_PLUGIN_STATIC_DECLARE(subenc); gstsubenc
// GST_PLUGIN_STATIC_DECLARE(switchbin);
// GST_PLUGIN_STATIC_DECLARE(timecode); gsttimecode
// GST_PLUGIN_STATIC_DECLARE(transcode);
// GST_PLUGIN_STATIC_DECLARE(videofiltersbad); gstvideofiltersbad
// GST_PLUGIN_STATIC_DECLARE(videoframe_audiolevel); gstvideoframe_audiolevel
GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
// GST_PLUGIN_STATIC_DECLARE(videosignal); gstvideosignal
// GST_PLUGIN_STATIC_DECLARE(vmnc); gstvmnc
// GST_PLUGIN_STATIC_DECLARE(y4mdec); gsty4mdec
// GST_PLUGIN_STATIC_DECLARE(decklink); gstdecklink
// GST_PLUGIN_STATIC_DECLARE(dvb); gstdvb
// GST_PLUGIN_STATIC_DECLARE(fbdevsink); gstfbdevsink
GST_PLUGIN_STATIC_DECLARE(ipcpipeline);
#ifdef ENABLENVCODEC
GST_PLUGIN_STATIC_DECLARE(nvcodec);
#endif
// GST_PLUGIN_STATIC_DECLARE(shm); gstshm
// GST_PLUGIN_STATIC_DECLARE(tinyalsa);
GST_PLUGIN_STATIC_DECLARE(v4l2codecs);
// GST_PLUGIN_STATIC_DECLARE(aes); gstaes
// GST_PLUGIN_STATIC_DECLARE(avtp); gstavtp
// GST_PLUGIN_STATIC_DECLARE(closedcaption); gstclosedcaption
// GST_PLUGIN_STATIC_DECLARE(dash); gstdash
// GST_PLUGIN_STATIC_DECLARE(dtls); gstdtls
GST_PLUGIN_STATIC_DECLARE(fdkaac);
// GST_PLUGIN_STATIC_DECLARE(hls); // gsthls
// GST_PLUGIN_STATIC_DECLARE(iqa);
// GST_PLUGIN_STATIC_DECLARE(microdns); gstmicrodns 
GST_PLUGIN_STATIC_DECLARE(openh264);
// GST_PLUGIN_STATIC_DECLARE(opusparse); gstopusparse
// GST_PLUGIN_STATIC_DECLARE(sctp);
// GST_PLUGIN_STATIC_DECLARE(smoothstreaming); gstsmoothstreaming
// GST_PLUGIN_STATIC_DECLARE(webrtc); gstwebrtcgstvmnc
GST_PLUGIN_STATIC_DECLARE(asf);
// GST_PLUGIN_STATIC_DECLARE(dvdlpcmdec); gstdvdlpcmdec
// GST_PLUGIN_STATIC_DECLARE(dvdsub); gstdvdsub
// GST_PLUGIN_STATIC_DECLARE(realmedia); gstrealmedia
// GST_PLUGIN_STATIC_DECLARE(x264);
#ifndef ENABLERPI
GST_PLUGIN_STATIC_DECLARE(pulseaudio);
#endif
// GST_PLUGIN_STATIC_DECLARE(rtspclientsink);
// GST_PLUGIN_STATIC_DECLARE(nle); gstnle
// GST_PLUGIN_STATIC_DECLARE(ges); gstges
// extern void g_io_environmentproxy_load (gpointer data);
// extern void g_io_openssl_load (gpointer data);

#endif
void
onvif_init_static_plugins (void)
{
  static gsize initialization_value = 0;
  if (g_once_init_enter (&initialization_value)) {

    C_DEBUG("Initializing Gstreamer plugins...");
    GST_PLUGIN_STATIC_REGISTER(gtknew);

#ifdef STATIC_BUILD
    
    C_DEBUG("Loading static plugins...");
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    // GST_PLUGIN_STATIC_REGISTER(coretracers);
    // GST_PLUGIN_STATIC_REGISTER(adder); gstadder
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(audioconvert);
    // GST_PLUGIN_STATIC_REGISTER(audiomixer); gstaudiomixer
    // GST_PLUGIN_STATIC_REGISTER(audiorate); gstaudiorate
    GST_PLUGIN_STATIC_REGISTER(audioresample);
    GST_PLUGIN_STATIC_REGISTER(audiotestsrc);
    // GST_PLUGIN_STATIC_REGISTER(compositor); gstcompositor
    // GST_PLUGIN_STATIC_REGISTER(encoding); gstencoding
    // GST_PLUGIN_STATIC_REGISTER(gio); gstgio
    GST_PLUGIN_STATIC_REGISTER(overlaycomposition);
    GST_PLUGIN_STATIC_REGISTER(pbtypes);
    GST_PLUGIN_STATIC_REGISTER(playback);
    GST_PLUGIN_STATIC_REGISTER(rawparse);
    // GST_PLUGIN_STATIC_REGISTER(subparse); gstsubparse
    GST_PLUGIN_STATIC_REGISTER(tcp);
    GST_PLUGIN_STATIC_REGISTER(typefindfunctions);
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
    // GST_PLUGIN_STATIC_REGISTER(videorate); gstvideorate
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(volume); // gstvolume
    GST_PLUGIN_STATIC_REGISTER(alsa);
    // GST_PLUGIN_STATIC_REGISTER(opengl); gstopengl
    // GST_PLUGIN_STATIC_REGISTER(ogg); gstogg
    // GST_PLUGIN_STATIC_REGISTER(opus); gstopus
    // GST_PLUGIN_STATIC_REGISTER(vorbis); gstvorbis
    GST_PLUGIN_STATIC_REGISTER(ximagesink);
    // GST_PLUGIN_STATIC_REGISTER(xvimagesink);
    // GST_PLUGIN_STATIC_REGISTER(alpha); gstalpha
    // GST_PLUGIN_STATIC_REGISTER(alphacolor); gstalphacolor
    // GST_PLUGIN_STATIC_REGISTER(apetag); gstapetag
    // GST_PLUGIN_STATIC_REGISTER(audiofx); gstaudiofx
    GST_PLUGIN_STATIC_REGISTER(audioparsers);
    // GST_PLUGIN_STATIC_REGISTER(auparse); gstauparse
    GST_PLUGIN_STATIC_REGISTER(autodetect);
    // GST_PLUGIN_STATIC_REGISTER(avi); gstavi
    // GST_PLUGIN_STATIC_REGISTER(cutter); gstcutter
    // GST_PLUGIN_STATIC_REGISTER(navigationtest); gstnavigationtest
    // GST_PLUGIN_STATIC_REGISTER(debug); gstdebug
    // GST_PLUGIN_STATIC_REGISTER(deinterlace); gstdeinterlace
    // GST_PLUGIN_STATIC_REGISTER(dtmf); gstdtmf
    // GST_PLUGIN_STATIC_REGISTER(effectv); gsteffectv
    // GST_PLUGIN_STATIC_REGISTER(equalizer); gstequalizer
    // GST_PLUGIN_STATIC_REGISTER(flv); gstflv
    // GST_PLUGIN_STATIC_REGISTER(flxdec); gstflxdec
    // GST_PLUGIN_STATIC_REGISTER(goom); gstgoom
    // GST_PLUGIN_STATIC_REGISTER(goom2k1); gstgoom2k1
    // GST_PLUGIN_STATIC_REGISTER(icydemux); gsticydemux
    // GST_PLUGIN_STATIC_REGISTER(id3demux); gstid3demux
    // GST_PLUGIN_STATIC_REGISTER(imagefreeze); gstimagefreeze
    GST_PLUGIN_STATIC_REGISTER(interleave); // gstinterleave
    // GST_PLUGIN_STATIC_REGISTER(isomp4); gstisomp4
    // GST_PLUGIN_STATIC_REGISTER(alaw); gstalaw
    GST_PLUGIN_STATIC_REGISTER(mulaw);
    GST_PLUGIN_STATIC_REGISTER(level); // gstlevel
    // GST_PLUGIN_STATIC_REGISTER(matroska); gstmatroska 
    // GST_PLUGIN_STATIC_REGISTER(monoscope); gstmonoscope 
    // GST_PLUGIN_STATIC_REGISTER(multifile); gstmultifile
    // GST_PLUGIN_STATIC_REGISTER(multipart); gstmultipart
#ifdef ENABLERPI
    GST_PLUGIN_STATIC_REGISTER(omx);
    GST_PLUGIN_STATIC_REGISTER(rpicamsrc);
#endif
#ifdef ENABLELIBAV
    GST_PLUGIN_STATIC_REGISTER(libav);
#endif
    // GST_PLUGIN_STATIC_REGISTER(replaygain); gstreplaygain
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(rtsp);
    // GST_PLUGIN_STATIC_REGISTER(shapewipe); gstshapewipe
    // GST_PLUGIN_STATIC_REGISTER(smpte); gstsmpte
    // GST_PLUGIN_STATIC_REGISTER(spectrum); gstspectrum
    GST_PLUGIN_STATIC_REGISTER(udp);
    // GST_PLUGIN_STATIC_REGISTER(videobox); gstvideobox
    // GST_PLUGIN_STATIC_REGISTER(videocrop); gstvideocrop
    // GST_PLUGIN_STATIC_REGISTER(videofilter); gstvideofilter
    // GST_PLUGIN_STATIC_REGISTER(videomixer); gstvideomixer
    // GST_PLUGIN_STATIC_REGISTER(wavenc); gstwavenc
    // GST_PLUGIN_STATIC_REGISTER(wavparse); gstwavparse
    // GST_PLUGIN_STATIC_REGISTER(xingmux); gstxingmux
    // GST_PLUGIN_STATIC_REGISTER(y4menc); gsty4menc
    // GST_PLUGIN_STATIC_REGISTER(ossaudio);
    // GST_PLUGIN_STATIC_REGISTER(oss4);
    GST_PLUGIN_STATIC_REGISTER(video4linux2);
    // GST_PLUGIN_STATIC_REGISTER(ximagesrc); gstximagesrc
    // GST_PLUGIN_STATIC_REGISTER(adaptivedemux2); gstadaptivedemux2
    // GST_PLUGIN_STATIC_REGISTER(cairo);
    // GST_PLUGIN_STATIC_REGISTER(gdkpixbuf); // gstgdkpixbuf
    // GST_PLUGIN_STATIC_REGISTER(gtk); // gstgtk
    GST_PLUGIN_STATIC_REGISTER(jpeg); // gstjpeg
    // GST_PLUGIN_STATIC_REGISTER(lame); gstlame 
    // GST_PLUGIN_STATIC_REGISTER(dv); gstdv
    // GST_PLUGIN_STATIC_REGISTER(png); gstpng
    // GST_PLUGIN_STATIC_REGISTER(soup); gstsoup
    // GST_PLUGIN_STATIC_REGISTER(nice); gstnice
    // GST_PLUGIN_STATIC_REGISTER(accurip); gstaccurip
    // GST_PLUGIN_STATIC_REGISTER(adpcmdec); gstadpcmdec
    // GST_PLUGIN_STATIC_REGISTER(adpcmenc); gstadpcmenc
    // GST_PLUGIN_STATIC_REGISTER(aiff);
    // GST_PLUGIN_STATIC_REGISTER(asfmux); gstasfmux
    // GST_PLUGIN_STATIC_REGISTER(audiobuffersplit); gstaudiobuffersplit
    // GST_PLUGIN_STATIC_REGISTER(audiofxbad); gstaudiofxbad
    // GST_PLUGIN_STATIC_REGISTER(audiomixmatrix); gstaudiomixmatrix
    // GST_PLUGIN_STATIC_REGISTER(audiolatency);
    // GST_PLUGIN_STATIC_REGISTER(audiovisualizers); gstaudiovisualizers
    // GST_PLUGIN_STATIC_REGISTER(autoconvert); gstautoconvert
    // GST_PLUGIN_STATIC_REGISTER(bayer); gstbayer
    // GST_PLUGIN_STATIC_REGISTER(camerabin); gstcamerabin
    // GST_PLUGIN_STATIC_REGISTER(codecalpha); gstcodecalpha
    // GST_PLUGIN_STATIC_REGISTER(codectimestamper); gstcodectimestamper
    // GST_PLUGIN_STATIC_REGISTER(coloreffects); gstcoloreffects
    // GST_PLUGIN_STATIC_REGISTER(debugutilsbad); gstdebugutilsbad
    // GST_PLUGIN_STATIC_REGISTER(dvbsubenc); gstdvbsubenc
    // GST_PLUGIN_STATIC_REGISTER(dvbsuboverlay); gstdvbsuboverlay
    // GST_PLUGIN_STATIC_REGISTER(dvdspu); gstdvdspu
    // GST_PLUGIN_STATIC_REGISTER(faceoverlay); gstfaceoverlay
    // GST_PLUGIN_STATIC_REGISTER(festival); gstfestival
    // GST_PLUGIN_STATIC_REGISTER(fieldanalysis); gstfieldanalysis
    // GST_PLUGIN_STATIC_REGISTER(freeverb); gstfreeverb
    // GST_PLUGIN_STATIC_REGISTER(frei0r); gstfrei0r
    // GST_PLUGIN_STATIC_REGISTER(gaudieffects); gstgaudieffects
    // GST_PLUGIN_STATIC_REGISTER(gdp); gstgdp
    // GST_PLUGIN_STATIC_REGISTER(geometrictransform); gstgeometrictransform
    // GST_PLUGIN_STATIC_REGISTER(id3tag); gstid3tag
    // GST_PLUGIN_STATIC_REGISTER(inter); gstinter
    // GST_PLUGIN_STATIC_REGISTER(interlace); gstinterlace
    // GST_PLUGIN_STATIC_REGISTER(ivfparse); gstivfparse
    // GST_PLUGIN_STATIC_REGISTER(ivtc); gstivtc
    // GST_PLUGIN_STATIC_REGISTER(jp2kdecimator); gstjp2kdecimator
    GST_PLUGIN_STATIC_REGISTER(jpegformat); // gstjpegformat
    // GST_PLUGIN_STATIC_REGISTER(rfbsrc); gstrfbsrc
    // GST_PLUGIN_STATIC_REGISTER(midi); gstmidi
    // GST_PLUGIN_STATIC_REGISTER(mpegpsdemux); gstmpegpsdemux
    // GST_PLUGIN_STATIC_REGISTER(mpegpsmux); gstmpegpsmux
    // GST_PLUGIN_STATIC_REGISTER(mpegtsdemux);
    // GST_PLUGIN_STATIC_REGISTER(mpegtsmux);
    // GST_PLUGIN_STATIC_REGISTER(mxf); gstmxf
    // GST_PLUGIN_STATIC_REGISTER(netsim); gstnetsim
    GST_PLUGIN_STATIC_REGISTER(rtponvif);
    // GST_PLUGIN_STATIC_REGISTER(pcapparse); gstpcapparse
    // GST_PLUGIN_STATIC_REGISTER(pnm); gstpnm
    // GST_PLUGIN_STATIC_REGISTER(proxy); gstproxy
    // GST_PLUGIN_STATIC_REGISTER(legacyrawparse);
    // GST_PLUGIN_STATIC_REGISTER(removesilence); gstremovesilence
    // GST_PLUGIN_STATIC_REGISTER(rist); gstrist
    // GST_PLUGIN_STATIC_REGISTER(rtmp2); gstrtmp2
    // GST_PLUGIN_STATIC_REGISTER(rtpmanagerbad); gstrtpmanagerbad
    // GST_PLUGIN_STATIC_REGISTER(sdpelem);
    // GST_PLUGIN_STATIC_REGISTER(segmentclip); gstsegmentclip
    // GST_PLUGIN_STATIC_REGISTER(siren); gstsiren
    // GST_PLUGIN_STATIC_REGISTER(smooth); gstsmooth
    // GST_PLUGIN_STATIC_REGISTER(speed); gstspeed
    // GST_PLUGIN_STATIC_REGISTER(subenc); gstsubenc
    // GST_PLUGIN_STATIC_REGISTER(switchbin);
    // GST_PLUGIN_STATIC_REGISTER(timecode); gsttimecode
    // GST_PLUGIN_STATIC_REGISTER(transcode);
    // GST_PLUGIN_STATIC_REGISTER(videofiltersbad); gstvideofiltersbad
    // GST_PLUGIN_STATIC_REGISTER(videoframe_audiolevel); gstvideoframe_audiolevel
    GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
    // GST_PLUGIN_STATIC_REGISTER(videosignal); gstvideosignal
    // GST_PLUGIN_STATIC_REGISTER(vmnc); gstvmnc
    // GST_PLUGIN_STATIC_REGISTER(y4mdec); gsty4mdec
    // GST_PLUGIN_STATIC_REGISTER(decklink); gstdecklink
    // GST_PLUGIN_STATIC_REGISTER(dvb); gstdvb
    // GST_PLUGIN_STATIC_REGISTER(fbdevsink); gstfbdevsink
    // GST_PLUGIN_STATIC_REGISTER(ipcpipeline); gstipcpipeline
#ifdef ENABLENVCODEC
    C_DEBUG("Using Nvida Codec...");
    GST_PLUGIN_STATIC_REGISTER(nvcodec);
#endif
    // GST_PLUGIN_STATIC_REGISTER(shm); gstshm
    // GST_PLUGIN_STATIC_REGISTER(tinyalsa);
    GST_PLUGIN_STATIC_REGISTER(v4l2codecs);
    // GST_PLUGIN_STATIC_REGISTER(aes); gstaes
    // GST_PLUGIN_STATIC_REGISTER(avtp); gstavtp
    // GST_PLUGIN_STATIC_REGISTER(closedcaption); gstclosedcaption
    // GST_PLUGIN_STATIC_REGISTER(dash); gstdash
    // GST_PLUGIN_STATIC_REGISTER(dtls); gstdtls
    GST_PLUGIN_STATIC_REGISTER(fdkaac);
    // GST_PLUGIN_STATIC_REGISTER(hls); // gsthls
    // GST_PLUGIN_STATIC_REGISTER(iqa);
    // GST_PLUGIN_STATIC_REGISTER(microdns); gstmicrodns 
    GST_PLUGIN_STATIC_REGISTER(openh264);
    // GST_PLUGIN_STATIC_REGISTER(opusparse); gstopusparse
    // GST_PLUGIN_STATIC_REGISTER(sctp);
    // GST_PLUGIN_STATIC_REGISTER(smoothstreaming); gstsmoothstreaming
    // GST_PLUGIN_STATIC_REGISTER(webrtc); gstwebrtcgstvmnc
    // GST_PLUGIN_STATIC_REGISTER(asf); gstasfmux
    // GST_PLUGIN_STATIC_REGISTER(dvdlpcmdec); gstdvdlpcmdec
    // GST_PLUGIN_STATIC_REGISTER(dvdsub); gstdvdsub
    // GST_PLUGIN_STATIC_REGISTER(realmedia); gstrealmedia
    // GST_PLUGIN_STATIC_REGISTER(x264);
#ifndef ENABLERPI
    GST_PLUGIN_STATIC_REGISTER(pulseaudio);
#endif
    // GST_PLUGIN_STATIC_REGISTER(rtspclientsink);
    // GST_PLUGIN_STATIC_REGISTER(nle); gstnle
    // GST_PLUGIN_STATIC_REGISTER(ges); gstges
    // g_io_environmentproxy_load (NULL);
    // g_io_openssl_load (NULL);
#endif
    g_once_init_leave (&initialization_value, 1);
    C_DEBUG("Gstreamer plugins initialized...");
  }
}