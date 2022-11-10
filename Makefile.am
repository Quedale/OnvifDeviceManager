ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = onvifclient onvifserver onvifmgr

onvifclient_SOURCES = $(top_srcdir)/src/onvif-client.c
onvifclient_CFLAGS = `PKG_CONFIG_PATH=$HOME/git/cerbero-main/build/dist/linux_x86_64/lib/pkgconfig pkg-config --cflags libffi gstreamer-1.0`
onvifclient_LDFLAGS = `PKG_CONFIG_PATH=$HOME/git/cerbero-main/build/dist/linux_x86_64/lib/pkgconfig pkg-config --libs libffi gstreamer-1.0`

onvifserver_SOURCES = $(top_srcdir)/src/onvif-server.c
onvifserver_CFLAGS = `PKG_CONFIG_PATH=$HOME/git/cerbero-main/build/dist/linux_x86_64/lib/pkgconfig pkg-config --cflags glib-2.0 gio-2.0 libffi gstreamer-video-1.0 gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0 gstreamer-rtp-1.0`
onvifserver_LDFLAGS = `PKG_CONFIG_PATH=$HOME/git/cerbero-main/build/dist/linux_x86_64/lib/pkgconfig pkg-config --libs glib-2.0 gio-2.0 libffi gstreamer-video-1.0 gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0 gstreamer-rtp-1.0`

onvifmgr_SOURCES = $(top_srcdir)/src/onvif-mgr.c $(top_srcdir)/src/gst2/player.c
onvifmgr_CFLAGS= `pkg-config --cflags gstreamer-rtsp-server-1.0 gstreamer-video-1.0 gtk+-3.0` -I$(top_srcdir)/OnvifDiscoveryLib/build/dist/include -I$(top_srcdir)/OnvifSoapLib/build/dist/include 
onvifmgr_LDFLAGS= -L$(top_srcdir)/OnvifDiscoveryLib/build/dist/lib -L$(top_srcdir)/OnvifSoapLib/build/dist/lib -pthread -lonvifsoap -lonvifdisco `pkg-config --libs gstreamer-rtsp-server-1.0 gstreamer-video-1.0 gtk+-3.0`
onvifmgr_LDADD = 


onvifsoaplib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd OnvifSoapLib && $(MAKE) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd OnvifDiscoveryLib && $(MAKE) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 
