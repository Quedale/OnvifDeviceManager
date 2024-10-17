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
EXTRA_PROGRAMS = gifdemo overlaytest queuedemo csssliderdemo playerdemo cssfilesliderdemo gtksliderdemo omgrdevicedemo gtkstyledimagedemo

playerdemo_SOURCES = $(abs_top_srcdir)/src/demo/player-demo.c \
					$(abs_top_srcdir)/src/alsa/alsa_devices.c \
					$(abs_top_srcdir)/src/alsa/alsa_utils.c \
					$(abs_top_srcdir)/src/gst/gst_plugin_utils.c \
					$(abs_top_srcdir)/src/gst/overlay.c \
					$(abs_top_srcdir)/src/gst/gstrtspplayer.c \
					$(abs_top_srcdir)/src/gst/src_retriever.c \
					$(abs_top_srcdir)/src/gst/gtk/gstplugin.c \
					$(abs_top_srcdir)/src/gst/gtk/gstgtkbasesink.c \
					$(abs_top_srcdir)/src/gst/gtk/gstgtksink.c \
					$(abs_top_srcdir)/src/gst/gtk/gstgtkutils.c \
					$(abs_top_srcdir)/src/gst/gtk/gtkgstbasewidget.c \
					$(abs_top_srcdir)/src/gst/gtk/gtkgstwidget.c \
					$(abs_top_srcdir)/src/gst/backchannel.c
playerdemo_CFLAGS = $(DEBUG_FLAG) -DHAVE_CONFIG_H -Wall $(GST_STATIC_FLAG) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS) gtk+-3.0 cutils` $(EXT_CFLAGS)
playerdemo_LDFLAGS = $(GST_LINK_TYPE) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS) gtk+-3.0 cutils` -Wl,-Bdynamic -lm -lstdc++ -z noexecstack


onvifmgr_SOURCES = $(abs_top_srcdir)/src/onvif-mgr.c \
					$(abs_top_srcdir)/src/alsa/alsa_devices.c \
					$(abs_top_srcdir)/src/alsa/alsa_utils.c \
					$(abs_top_srcdir)/src/animations/gtk/custom_gtk_revealer.c \
					$(abs_top_srcdir)/src/animations/gtk/custom_gtk_progress_tracker.c \
					$(abs_top_srcdir)/src/animations/gtk/gtk_dotted_slider_widget.c \
					$(abs_top_srcdir)/src/app/gui_utils.c \
					$(abs_top_srcdir)/src/app/omgr_device_row.c \
					$(abs_top_srcdir)/src/app/onvif_app_shutdown.c \
					$(abs_top_srcdir)/src/app/onvif_app.c \
					$(abs_top_srcdir)/src/app/details/onvif_details.c \
					$(abs_top_srcdir)/src/app/details/onvif_info.c \
					$(abs_top_srcdir)/src/app/details/onvif_network.c \
					$(abs_top_srcdir)/src/app/onvif_nvt.c \
					$(abs_top_srcdir)/src/app/task_manager.c \
					$(abs_top_srcdir)/src/app/dialog/add_device.c \
					$(abs_top_srcdir)/src/app/dialog/app_dialog.c \
					$(abs_top_srcdir)/src/app/dialog/credentials_input.c \
					$(abs_top_srcdir)/src/app/dialog/gtkprofilepanel.c \
					$(abs_top_srcdir)/src/app/dialog/msg_dialog.c \
					$(abs_top_srcdir)/src/app/dialog/profiles_dialog.c \
					$(abs_top_srcdir)/src/app/settings/app_settings_discovery.c \
					$(abs_top_srcdir)/src/app/settings/app_settings_stream.c \
					$(abs_top_srcdir)/src/app/settings/app_settings.c \
					$(abs_top_srcdir)/src/app/c_ownable_interface.c \
					$(abs_top_srcdir)/src/app/gtkbinaryimage.c \
					$(abs_top_srcdir)/src/app/gtkstyledimage.c \
					$(abs_top_srcdir)/src/gst/gst_plugin_utils.c \
					$(abs_top_srcdir)/src/gst/overlay.c \
					$(abs_top_srcdir)/src/gst/src_retriever.c \
					$(abs_top_srcdir)/src/gst/backchannel.c \
					$(abs_top_srcdir)/src/gst/gstrtspplayer.c \
					$(abs_top_srcdir)/src/queue/event_queue.c \
					$(abs_top_srcdir)/src/queue/queue_event.c \
					$(abs_top_srcdir)/src/queue/queue_thread.c
onvifmgr_CFLAGS = $(DEBUG_FLAG) -Wall -Wextra -Wpedantic -Wno-unused-parameter $(DEBUG_FLAG) -DONVIFMGR_VERSION_MAJ=$(APP_VERSION_MAJ) -DONVIFMGR_VERSION_MIN=$(APP_VERSION_MIN) -DHAVE_CONFIG_H $(GST_STATIC_FLAG) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS) gtk+-3.0 libntlm cutils onvifsoap libssl libcrypto` $(EXT_CFLAGS)
onvifmgr_LDFLAGS = $(GST_LINK_TYPE) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS) gtk+-3.0 libntlm cutils onvifsoap libssl libcrypto` -Wl,-Bdynamic -lm -lstdc++ -z noexecstack
onvifmgr_LDADD = locked-icon.o microphone.o warning.o save.o tower.o

queuedemo_SOURCES = $(abs_top_srcdir)/src/demo/queue-demo.c $(abs_top_srcdir)/src/queue/event_queue.c $(abs_top_srcdir)/src/queue/queue_event.c $(abs_top_srcdir)/src/queue/queue_thread.c
queuedemo_LDFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs cutils glib-2.0 gobject-2.0`
queuedemo_CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags cutils glib-2.0 gobject-2.0`

gifdemo_SOURCES = $(abs_top_srcdir)/src/demo/gtk-gif.c
gifdemo_CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gtk+-3.0`
gifdemo_LDFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs gtk+-3.0`
gifdemo_LDADD = loading.o

gtkstyledimagedemo_SOURCES = $(abs_top_srcdir)/src/demo/gtkstyledimagedemo.c \
							$(abs_top_srcdir)/src/app/gtkstyledimage.c \
							$(abs_top_srcdir)/src/app/gtkbinaryimage.c \
							$(abs_top_srcdir)/src/app/gui_utils.c
gtkstyledimagedemo_CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gtk+-3.0 cutils`
gtkstyledimagedemo_LDFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs gtk+-3.0 cutils`
gtkstyledimagedemo_LDADD = microphone.o

overlaytest_SOURCES = $(abs_top_srcdir)/src/demo/overlay-demo.c
overlaytest_CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS)`
overlaytest_LDFLAGS = -Wl,-Bstatic `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(GST_PLGS)` -Wl,-Bdynamic

cssfilesliderdemo_SOURCES = $(abs_top_srcdir)/src/animations/dotted_slider.c $(abs_top_srcdir)/src/demo/css/slider/dotted_slider_demo.c
cssfilesliderdemo_CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gtk+-3.0`
cssfilesliderdemo_LDFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs gtk+-3.0`

csssliderdemo_SOURCES = $(abs_top_srcdir)/src/demo/slider-demo.c \
					$(abs_top_srcdir)/src/animations/gtk/css_dotted_slider.c \
					$(abs_top_srcdir)/src/animations/gtk/custom_gtk_revealer.c \
					$(abs_top_srcdir)/src/animations/gtk/custom_gtk_progress_tracker.c
csssliderdemo_CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gtk+-3.0` -Wl,-Bdynamic -lm -lstdc++ -z noexecstack
csssliderdemo_LDFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs gtk+-3.0`


omgrdevicedemo_SOURCES = $(abs_top_srcdir)/src/demo/omgr-device-demo.c \
						$(abs_top_srcdir)/src/app/gui_utils.c \
						$(abs_top_srcdir)/src/app/c_ownable_interface.c \
						$(abs_top_srcdir)/src/app/gtkstyledimage.c \
						$(abs_top_srcdir)/src/app/gtkbinaryimage.c \
					 	$(abs_top_srcdir)/src/app/omgr_device_row.c
omgrdevicedemo_CFLAGS = $(DEBUG_FLAG) -Wall -Wextra -Wpedantic -Wno-unused-parameter $(GST_STATIC_FLAG) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags $(GST_LIBS) $(GST_PLGS) gtk+-3.0 cutils onvifsoap libntlm`
omgrdevicedemo_LDFLAGS = $(GST_LINK_TYPE) $(LIB_UDEV_PATH) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs $(GST_LIBS) $(EXT_PLGS) $(GST_PLGS) gtk+-3.0 cutils onvifsoap libntlm` -Wl,-Bdynamic -lm -lstdc++ -z noexecstack
omgrdevicedemo_LDADD = save.o locked-icon.o warning.o

gtksliderdemo_SOURCES = $(abs_top_srcdir)/src/demo/widget-demo.c \
					$(abs_top_srcdir)/src/animations/gtk/gtk_dotted_slider_widget.c \
					$(abs_top_srcdir)/src/animations/gtk/custom_gtk_revealer.c \
					$(abs_top_srcdir)/src/animations/gtk/custom_gtk_progress_tracker.c
gtksliderdemo_CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter $(DEBUG_FLAG) `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --cflags gtk+-3.0 cutils`
gtksliderdemo_LDFLAGS = `PKG_CONFIG_PATH=$(PKG_FULL_PATH) pkg-config --libs gtk+-3.0 cutils` -Wl,-Bdynamic -lm -lstdc++ -z noexecstack

deb$(EXEEXT):
	@echo "Creating debian packge definition..."
	@rm -rf onvifmgr_$(APP_VERSION)
	@mkdir onvifmgr_$(APP_VERSION)
	@mkdir onvifmgr_$(APP_VERSION)/usr
	@mkdir onvifmgr_$(APP_VERSION)/usr/local
	@mkdir onvifmgr_$(APP_VERSION)/usr/local/bin
	@cp "$(abs_builddir)/onvifmgr" onvifmgr_$(APP_VERSION)/usr/local/bin
	@mkdir onvifmgr_$(APP_VERSION)/DEBIAN
	@echo "Package: onvifmgr" > onvifmgr_$(APP_VERSION)/DEBIAN/control
	@echo "Version: 0.0-1" >> onvifmgr_$(APP_VERSION)/DEBIAN/control
	@echo "Section: base"  >> onvifmgr_$(APP_VERSION)/DEBIAN/control
	@echo "Priority: optional" >> onvifmgr_$(APP_VERSION)/DEBIAN/control
	@echo "Architecture: i386" >> onvifmgr_$(APP_VERSION)/DEBIAN/control
	@echo "Maintainer: Your Name <you@email.com>" >> onvifmgr_$(APP_VERSION)/DEBIAN/control
	@echo "Description: Onvif Device Manager for Linux" >> onvifmgr_$(APP_VERSION)/DEBIAN/control
	@dpkg-deb --build onvifmgr_$(APP_VERSION)
	@echo ""
	@echo "*******************************************"
	@echo  "*"
	@echo  "* WARNING: This package is tailored for this computer"
	@echo  "*          Distro package dependencies are not yet properly configured"
	@echo  "*"
	@echo  "* To install, run 'sudo dpkg -i onvifmgr_$(APP_VERSION).deb'"
	@echo  "*"
	@echo "*******************************************"

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

tower.o$(EXEEXT):
	@echo "  CCLD     tower.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/tower.o tower.png && cd ..

microphone.o$(EXEEXT):
	@echo "  CCLD     microphone.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/microphone.o microphone.png && cd ..

locked-icon.o$(EXEEXT):
	@echo "  CCLD     locked-icon.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/locked-icon.o locked-icon.png && cd ..

warning.o$(EXEEXT):
	@echo "  CCLD     warning.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/warning.o warning.png && cd ..

save.o$(EXEEXT):
	@echo "  CCLD     save.o"
	@cd $(abs_top_srcdir)/images && ld -r -b binary -o $(abs_builddir)/save.o save.png && cd ..

uninstall-onvifsoaplib$(EXEEXT):
	cd $(abs_top_srcdir)/subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) uninstall GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

install-onvifsoaplib$(EXEEXT):
	cd $(abs_top_srcdir)/subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) install GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

clean-onvifsoaplib$(EXEEXT):
	cd $(abs_top_srcdir)/subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) clean GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

onvifsoaplib$(EXEEXT):
	echo "soap dir : " $(GSOAP_SRC_DIR)
	cd $(abs_top_srcdir)/subprojects/OnvifSoapLib && $(MAKE) $(JOB_FLAG) GSOAP_SRC_DIR=$(GSOAP_SRC_DIR) && cd ..

clean-gsoaplib$(EXEEXT):
	cd $(GSOAP_SRC_DIR) && $(MAKE) $(JOB_FLAG) clean && cd .. 

gsoaplib$(EXEEXT):
	cd $(GSOAP_SRC_DIR) && $(MAKE) $(JOB_FLAG) && cd .. 