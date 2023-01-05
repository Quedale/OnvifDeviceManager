ACLOCAL_AMFLAGS = -I m4
MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))
TAB  := $(NULL)<tab>$(NULL)

# Extract list of compiled package
GST_PLGS = $(shell ls $(PKG_GST_PATH) | sed -z 's/\.pc\n/ /g;s/,$$/\n/')
GST_LIBS = $(shell ls $(PKG_PATH) | sed -z 's/\.pc\n/ /g;s/,$$/\n/')

AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = onvifmgr 
EXTRA_PROGRAMS = onvifclient onvifserver giftest overlaytest queuetest

onvifclient_SOURCES = $(top_srcdir)/src/onvif-client.c $(top_srcdir)/src/gst2/onvifinitstaticplugins.c
onvifclient_CFLAGS = -Wall $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)` $(EXT_CFLAGS)
onvifclient_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS)` $(EXT_STC_LIBS) -Wl,-Bdynamic $(EXT_DYN_LIBS) -lstdc++ -L/usr/lib/x86_64-linux-gnu -lnvcuvid -lnvidia-encode

onvifserver_SOURCES = $(top_srcdir)/src/onvif-server.c $(top_srcdir)/src/gst2/onvifinitstaticplugins.c  $(top_srcdir)/src/gst2/sink-retriever.c
onvifserver_CFLAGS = -Wall $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)` $(EXT_CFLAGS)
onvifserver_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS)` $(EXT_STC_LIBS) -Wl,-Bdynamic $(EXT_DYN_LIBS) -lstdc++

onvifmgr_SOURCES = $(top_srcdir)/src/onvif-mgr.c $(top_srcdir)/src/gst2/player.c $(top_srcdir)/src/gst2/overlay.c $(top_srcdir)/src/queue/event_queue.c $(top_srcdir)/src/gui/gui_update_calls.c $(top_srcdir)/src/gui/credentials_input.c $(top_srcdir)/src/gst2/onvifinitstaticplugins.c 
onvifmgr_CFLAGS = -Wall $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS) gtk+-3.0` $(EXT_CFLAGS) -I$(top_srcdir)/build/dist/include
onvifmgr_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS) gtk+-3.0` $(EXT_STC_LIBS) -Wl,-Bdynamic $(EXT_DYN_LIBS) -lstdc++ -L$(top_srcdir)/build/dist/lib  -lonvifsoap -lonvifdisco -z noexecstack
onvifmgr_LDADD = locked-icon.o microphone.o

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

uninstall-onvifsoaplib$(EXEEXT):
	cd subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) uninstall GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

install-onvifsoaplib$(EXEEXT):
	cd subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) install GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

clean-onvifsoaplib$(EXEEXT):
	cd subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) clean GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

onvifsoaplib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

uninstall-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd subprojects/OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) uninstall GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

install-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd subprojects/OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) install GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 
	
clean-discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd subprojects/OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) clean GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

discolib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd subprojects/OnvifDiscoveryLib && $(MAKE) $(JOB_FLAG) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd .. 

clean-gsoaplib$(EXEEXT):
	cd $(GSOAP_SRC_DIR) && $(MAKE) $(JOB_FLAG) clean && cd .. 

gsoaplib$(EXEEXT):
	cd $(GSOAP_SRC_DIR) && $(MAKE) $(JOB_FLAG) && cd .. 