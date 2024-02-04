ACLOCAL_AMFLAGS = -I m4
MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))
TAB  := $(NULL)<tab>$(NULL)

# Extract list of compiled package
# GST_PLGS = $(shell ls $(PKG_GST_PATH) | sed -z 's/\.pc\n/ /g;s/,$$/\n/')
# GST_LIBS = $(shell ls $(PKG_PATH) | sed -z 's/\.pc\n/ /g;s/,$$/\n/')

AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = onvifmgr 
EXTRA_PROGRAMS = onvifclient onvifserver giftest overlaytest queuetest dottedslider clogger playertest

playertest_SOURCES = $(top_srcdir)/sample/player-test.c \
					$(top_srcdir)/src/alsa/alsa_devices.c \
					$(top_srcdir)/src/alsa/alsa_utils.c \
					$(top_srcdir)/src/gst/onvifinitstaticplugins.c \
					$(top_srcdir)/src/gst/overlay.c \
					$(top_srcdir)/src/gst/player.c \
					$(top_srcdir)/src/gst/src_retriever.c \
					$(top_srcdir)/src/gst/gtk/gstplugin.c \
					$(top_srcdir)/src/gst/gtk/gstgtkbasesink.c \
					$(top_srcdir)/src/gst/gtk/gstgtksink.c \
					$(top_srcdir)/src/gst/gtk/gstgtkutils.c \
					$(top_srcdir)/src/gst/gtk/gtkgstbasewidget.c \
					$(top_srcdir)/src/gst/gtk/gtkgstwidget.c \
					$(top_srcdir)/src/gst/backchannel.c
playertest_CFLAGS = $(DEBUG_FLAG) -DHAVE_CONFIG_H -Wall $(GST_STATIC_FLAG) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS) gtk+-3.0 cutils onvifdisco onvifsoap` $(EXT_CFLAGS) -I$(top_srcdir)/build/dist/include
playertest_LDFLAGS = $(GST_LINK_TYPE) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS) gtk+-3.0 cutils onvifdisco onvifsoap` -Wl,-Bdynamic -lm -lstdc++ -z noexecstack

onvifclient_SOURCES = $(top_srcdir)/src/onvif-client.c $(top_srcdir)/src/gst/onvifinitstaticplugins.c $(top_srcdir)/src/gst/gtk/gstgtkbasesink.c $(top_srcdir)/src/gst/gtk/gstgtksink.c $(top_srcdir)/src/gst/gtk/gstgtkutils.c $(top_srcdir)/src/gst/gtk/gtkgstbasewidget.c $(top_srcdir)/src/gst/gtk/gtkgstwidget.c $(top_srcdir)/src/gst/gtk/gstplugin.c
onvifclient_CFLAGS = -DHAVE_CONFIG_H -Wall $(GST_STATIC_FLAG) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS) gtk+-3.0` $(EXT_CFLAGS)
onvifclient_LDFLAGS = $(GST_LINK_TYPE) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS) gtk+-3.0` -Wl,-Bdynamic -lstdc++ -L/usr/lib/x86_64-linux-gnu -lnvcuvid -lnvidia-encode

onvifserver_SOURCES = $(top_srcdir)/src/onvif-server.c $(top_srcdir)/src/gst/onvifinitstaticplugins.c  $(top_srcdir)/src/gst/sink-retriever.c
onvifserver_CFLAGS = -Wall $(GST_STATIC_FLAG) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)` $(EXT_CFLAGS)
onvifserver_LDFLAGS = $(GST_LINK_TYPE) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS)` -Wl,-Bdynamic -lstdc++

onvifmgr_SOURCES = $(top_srcdir)/src/onvif-mgr.c \
					$(top_srcdir)/src/alsa/alsa_devices.c \
					$(top_srcdir)/src/alsa/alsa_utils.c \
					$(top_srcdir)/src/animations/dotted_slider.c \
					$(top_srcdir)/src/app/device.c \
					$(top_srcdir)/src/app/gui_utils.c \
					$(top_srcdir)/src/app/onvif_app_shutdown.c \
					$(top_srcdir)/src/app/onvif_app.c \
					$(top_srcdir)/src/app/onvif_details.c \
					$(top_srcdir)/src/app/onvif_info.c \
					$(top_srcdir)/src/app/onvif_network.c \
					$(top_srcdir)/src/app/onvif_nvt.c \
					$(top_srcdir)/src/app/task_manager.c \
					$(top_srcdir)/src/app/dialog/add_device.c \
					$(top_srcdir)/src/app/dialog/app_dialog.c \
					$(top_srcdir)/src/app/dialog/credentials_input.c \
					$(top_srcdir)/src/app/dialog/msg_dialog.c \
					$(top_srcdir)/src/app/settings/app_settings_discovery.c \
					$(top_srcdir)/src/app/settings/app_settings_stream.c \
					$(top_srcdir)/src/app/settings/app_settings.c \
					$(top_srcdir)/src/gst/onvifinitstaticplugins.c \
					$(top_srcdir)/src/gst/overlay.c \
					$(top_srcdir)/src/gst/player.c \
					$(top_srcdir)/src/gst/src_retriever.c \
					$(top_srcdir)/src/gst/gtk/gstplugin.c \
					$(top_srcdir)/src/gst/gtk/gstgtkbasesink.c \
					$(top_srcdir)/src/gst/gtk/gstgtksink.c \
					$(top_srcdir)/src/gst/gtk/gstgtkutils.c \
					$(top_srcdir)/src/gst/gtk/gtkgstbasewidget.c \
					$(top_srcdir)/src/gst/gtk/gtkgstwidget.c \
					$(top_srcdir)/src/gst/backchannel.c \
					$(top_srcdir)/src/queue/event_queue.c \
					$(top_srcdir)/src/queue/queue_event.c \
					$(top_srcdir)/src/queue/queue_thread.c

onvifmgr_CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter $(DEBUG_FLAG) -DHAVE_CONFIG_H $(GST_STATIC_FLAG) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS) gtk+-3.0 cutils onvifsoap onvifdisco` $(EXT_CFLAGS) -I$(top_srcdir)/build/dist/include -Wall -Wextra -Wpedantic 
onvifmgr_LDFLAGS = $(GST_LINK_TYPE) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS) gtk+-3.0 cutils onvifsoap onvifdisco` -Wl,-Bdynamic -lm -lstdc++ -z noexecstack
onvifmgr_LDADD = locked-icon.o microphone.o warning.o

queuetest_SOURCES = $(top_srcdir)/src/demo/queue_test.c $(top_srcdir)/src/queue/event_queue.c
queuetest_CFLAGS = -Wall -lpthread -Wall 

giftest_SOURCES = $(top_srcdir)/src/demo/gtk-gif.c
giftest_CFLAGS = -Wall `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gtk+-3.0`
giftest_LDFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs gtk+-3.0`
giftest_LDADD = loading.o

overlaytest_SOURCES = $(top_srcdir)/src/demo/overlay-demo.c
overlaytest_CFLAGS = -Wall `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)`  -I$(top_srcdir)/build/dist/include
overlaytest_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(GST_PLGS)` -Wl,-Bdynamic
overlaytest_LDADD =

dottedslider_SOURCES = $(top_srcdir)/src/animations/dotted_slider.c $(top_srcdir)/src/demo/css/slider/dotted_slider_demo.c
dottedslider_CFLAGS = -Wall `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gtk+-3.0`
dottedslider_LDFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs gtk+-3.0`


paths$(EXEEXT):
	@echo "PKG_FULL_PATH : $(PKG_FULL_PATH)"
	@echo ""
	@echo "GST_PLGS : $(GST_PLGS)"
	@echo ""
	@echo "GST_LIBS : $(GST_LIBS)"

printinc$(EXEEXT):
	@`gcc -print-prog-name=cc1` -v

pkgpath$(EXEEXT):
	@echo "PKG_FULL_PATH : $(PKG_FULL_PATH)"
	@echo "CFLAGS : " `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gio-2.0`

loading.o$(EXEEXT):
	@echo "  CCLD     loading.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/loading.o loading.gif && cd ..

microphone.o$(EXEEXT):
	@echo "  CCLD     microphone.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/microphone.o microphone.png && cd ..

locked-icon.o$(EXEEXT):
	@echo "  CCLD     locked-icon.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/locked-icon.o locked-icon.png && cd ..

warning.o$(EXEEXT):
	@echo "  CCLD     warning.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/warning.o warning.png && cd ..

uninstall-onvifsoaplib$(EXEEXT):
	cd $(abs_top_srcdir)/subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) uninstall GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

install-onvifsoaplib$(EXEEXT):
	cd $(abs_top_srcdir)/subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) install GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

clean-onvifsoaplib$(EXEEXT):
	cd $(abs_top_srcdir)/subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) clean GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

onvifsoaplib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd $(abs_top_srcdir)/subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

uninstall-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd $(abs_top_srcdir)/subprojects/OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) uninstall GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

install-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd $(abs_top_srcdir)/subprojects/OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) install GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 
	
clean-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd $(abs_top_srcdir)/subprojects/OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) clean GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd $(abs_top_srcdir)/subprojects/OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

clean-gsoaplib$(EXEEXT):
	cd $(GSOAP_SRC_DIR) && $(MAKE) $(JOB_FLAG) clean && cd .. 

gsoaplib$(EXEEXT):
	cd $(GSOAP_SRC_DIR) && $(MAKE) $(JOB_FLAG) && cd .. 