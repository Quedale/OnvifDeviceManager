AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = onvifclient onvifserver onvifmgr
onvifclient_SOURCES = onvif-client.c
onvifserver_SOURCES = onvif-server.c
onvifmgr_SOURCES = onvif-mgr.c


onvifmgr$(EXEEXT):
	@echo "Compiling onvif-mgr..."
	@mkdir -p build
	@gcc `pkg-config --cflags gstreamer-rtsp-server-1.0` \
		`pkg-config --cflags gstreamer-video-1.0` \
		`pkg-config --cflags gtk+-3.0` \
		`pkg-config --cflags openssl` \
		-LOnvifDiscoveryLib/build/lib \
		-LOnvifSoapLib/build/lib \
		src/onvif-mgr.c \
		src/gst2/player.c \
		-o build/onvif-mgr \
			-pthread \
			-lonvif \
			-lonvif_discovery \
			`pkg-config --libs gstreamer-rtsp-server-1.0` \
			`pkg-config --libs gstreamer-video-1.0` \
			`pkg-config --libs gtk+-3.0` \
			`pkg-config --libs openssl` 


onvifserver$(EXEEXT):
	@echo "Compiling onvif-server..."
	@mkdir -p build
	@PKG_CONFIG_PATH=/home/quedale/git/cerbero-main/build/dist/linux_x86_64/lib/pkgconfig gcc \
		`pkg-config --cflags gstreamer-rtsp-1.0` \
		`pkg-config --cflags gstreamer-rtp-1.0` \
		`pkg-config --cflags gstreamer-rtsp-server-1.0` \
		`pkg-config --cflags gstreamer-video-1.0` \
		`pkg-config --cflags libffi` \
		`pkg-config --cflags gio-2.0` \
		`pkg-config --cflags glib-2.0` \
		src/onvif-server.c \
		-o build/onvif-server \
			`pkg-config --libs gstreamer-rtsp-1.0` \
			`pkg-config --libs gstreamer-rtp-1.0` \
			`pkg-config --libs gstreamer-rtsp-server-1.0` \
			`pkg-config --libs gstreamer-video-1.0` \
			`pkg-config --libs libffi` \
			`pkg-config --libs gio-2.0` \
			`pkg-config --libs glib-2.0` 


onvifclient$(EXEEXT):
	@echo "Compiling onvif-client..."
	@mkdir -p build
	@PKG_CONFIG_PATH=/home/quedale/git/cerbero-main/build/dist/linux_x86_64/lib/pkgconfig gcc \
		`pkg-config --cflags gstreamer-1.0` \
		`pkg-config --cflags libffi` \
		src/onvif-client.c \
		-o build/onvif-client \
			`pkg-config --libs gstreamer-1.0` \
			`pkg-config --libs libffi`
