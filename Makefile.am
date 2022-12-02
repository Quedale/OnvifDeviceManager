ACLOCAL_AMFLAGS = -I m4
MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))
TAB  := $(NULL)<tab>$(NULL)

AUTOMAKE_OPTIONS = foreign subdir-objects


bin_PROGRAMS = onvifclient onvifserver onvifmgr giftest overlaytest

onvifclient_SOURCES = $(top_srcdir)/src/onvif-client.c
onvifclient_CFLAGS = `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --cflags libffi gstreamer-1.0`
onvifclient_LDFLAGS = `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --libs libffi gstreamer-1.0`

onvifserver_SOURCES = $(top_srcdir)/src/onvif-server.c
onvifserver_CFLAGS = `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --cflags glib-2.0 gio-2.0 libffi gstreamer-video-1.0 gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0 gstreamer-rtp-1.0`
onvifserver_LDFLAGS = `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --libs glib-2.0 gio-2.0 libffi gstreamer-video-1.0 gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0 gstreamer-rtp-1.0`

onvifmgr_SOURCES = $(top_srcdir)/src/onvif-mgr.c $(top_srcdir)/src/gst2/player.c $(top_srcdir)/src/gst2/overlay.c $(top_srcdir)/src/queue/event_queue.c $(top_srcdir)/src/gui/gui_update_calls.c
onvifmgr_CFLAGS= `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --cflags gstreamer-rtsp-server-1.0 gstreamer-video-1.0 cairo gtk+-3.0 wayland-client glib-2.0 gio-2.0` -I$(top_srcdir)/build/dist/include -I$(top_srcdir)/build/dist/include 
onvifmgr_LDFLAGS= `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --libs gstreamer-rtsp-server-1.0 gstreamer-video-1.0 cairo gtk+-3.0 wayland-client glib-2.0 gio-2.0` -L$(top_srcdir)/build/dist/lib -lpthread -lonvifsoap -lonvifdisco 
onvifmgr_LDADD = locked-icon.o

queuetest_SOURCES = $(top_srcdir)/src/demo/queue_test.c $(top_srcdir)/src/queue/event_queue.c
queuetest_CFLAGS = -lpthread -Wall 

giftest_SOURCES = $(top_srcdir)/src/demo/gtk-gif.c
giftest_CFLAGS = -Wall `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --cflags gtk+-3.0`
giftest_LDFLAGS = `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --libs gtk+-3.0`
giftest_LDADD = loading.o

overlaytest_SOURCES = $(top_srcdir)/src/demo/overlay-demo.c
overlaytest_CFLAGS = `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --cflags gtk+-3.0 gstreamer-rtsp-server-1.0  gstreamer-video-1.0`
overlaytest_LDFLAGS = -lm `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --libs gtk+-3.0 gstreamer-rtsp-server-1.0  gstreamer-video-1.0`
overlaytest_LDADD =

printinc$(EXEEXT):
	@`gcc -print-prog-name=cc1` -v

pkgpath$(EXEEXT):
	@echo "CERBERO_PKG : $(CERBERO_PKG)"
	@echo "CERBERO FLAGS : " `PKG_CONFIG_PATH=$(CERBERO_PKG) pkg-config --cflags gio-2.0`

loading.o$(EXEEXT):
	@echo "  CCLD     loading.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/loading.o loading.gif && cd ..

locked-icon.o$(EXEEXT):
	@echo "  CCLD     locked-icon.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/locked-icon.o locked-icon.png && cd ..

uninstall-onvifsoaplib$(EXEEXT):
	cd OnvifSoapLib && $(MAKE) $(JOB_FLAG) uninstall GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

install-onvifsoaplib$(EXEEXT):
	cd OnvifSoapLib && $(MAKE) $(JOB_FLAG) install GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

clean-onvifsoaplib$(EXEEXT):
	cd OnvifSoapLib && $(MAKE) $(JOB_FLAG) clean GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

onvifsoaplib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd OnvifSoapLib && $(MAKE) $(JOB_FLAG) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

uninstall-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) uninstall GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

install-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) install GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 
	
clean-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) clean GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

clean-gsoaplib$(EXEEXT):
	cd $(GSOAP_SRC_DIR) && $(MAKE) $(JOB_FLAG) clean && cd .. 

gsoaplib$(EXEEXT):
	cd $(GSOAP_SRC_DIR) && $(MAKE) $(JOB_FLAG) && cd .. 