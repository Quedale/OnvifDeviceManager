AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = soaptest onvifclient onvifserver onvifmgr
soaptest_SOURCES = soap-test.c
onvifclient_SOURCES = onvif-client.c
onvifserver_SOURCES = onvif-server.c
onvifmgr_SOURCES = onvif-mgr.c


onvifmgr$(EXEEXT):
	@echo "Compiling onvif-mgr..."
	@mkdir -p build
	@gcc \
		`pkg-config --cflags gstreamer-rtsp-server-1.0` \
		`pkg-config --cflags gstreamer-video-1.0` \
		`pkg-config --cflags gtk+-3.0` \
		`pkg-config --cflags libxml-2.0` \
		`pkg-config --cflags gsoap` \
		src/onvif-mgr.c \
		src/gst2/player.c \
		src/udp/discovery.c \
		src/udp/soap_parser.c \
		src/soap/client.c \
		src/generated/soapClient.c \
		src/generated/soapC.c \
		src/ui/onvif_list.c \
		src/ui/onvif_device.c \
		-o build/onvif-mgr \
			`pkg-config --libs gstreamer-rtsp-server-1.0` \
			`pkg-config --libs gstreamer-video-1.0` \
			`pkg-config --libs gtk+-3.0` \
			`pkg-config --libs libxml-2.0` \
			`pkg-config --libs gsoap`


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

soaptest$(EXEEXT):
	@echo "Compiling soap-test..."
	@mkdir -p build
	@gcc `pkg-config --cflags gsoap` src/soap-test.c src/generated/soapClient.c src/generated/soapC.c -I/usr/share/gsoap/import \
		-o build/soap-test `pkg-config --libs gsoap`

guitest$(EXEEXT):
	@echo "Compiling gui-test..."
	@mkdir -p build
	@gcc 	`pkg-config --cflags gsoap` \
			`pkg-config --cflags gtk+-3.0` \
			`pkg-config --cflags libxml-2.0` \
			`pkg-config --cflags gstreamer-1.0` \
			`pkg-config --cflags gstreamer-video-1.0` \
			`pkg-config --cflags libffi` \
			src/soap/client.c \
			src/generated/soapClient.c \
			src/generated/soapC.c \
			src/gui-test.c \
			src/udp/soap_parser.c \
			src/udp/discovery.c \
			src/ui/onvif_list.c \
			src/gst/player.c \
		-o build/gui-test \
			`pkg-config --libs gtk+-3.0` \
			`pkg-config --libs cairo` \
			`pkg-config --libs libxml-2.0` \
			`pkg-config --libs gsoap` \
			`pkg-config --libs gstreamer-1.0` \
			`pkg-config --libs gstreamer-video-1.0` \
			`pkg-config --libs libffi`