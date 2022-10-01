AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = guitest soaptest onvifclient onvifserver
guitest_SOURCES = gui-test.c
soaptest_SOURCES = soap-test.c
onvifclient_SOURCES = onvif-client.c
onvifserver_SOURCES = onvif-server.c

onvifserver$(EXEEXT):
	@echo "Compiling onvif-server..."
	@mkdir -p build
	@gcc `pkg-config --cflags gstreamer-rtsp-server-1.0` src/onvif-server.c \
		-o build/onvif-server `pkg-config --libs gstreamer-rtsp-server-1.0`


onvifclient$(EXEEXT):
	@echo "Compiling onvif-client..."
	@mkdir -p build
	@gcc `pkg-config --cflags gstreamer-1.0` src/onvif-client.c \
		-o build/onvif-client `pkg-config --libs gstreamer-1.0`

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
			src/soap/client.c \
			src/generated/soapClient.c \
			src/generated/soapC.c \
			src/gui-test.c \
			src/udp/soap_parser.c \
			src/udp/discovery.c \
			src/ui/onvif_list.c \
		-o build/gui-test \
			`pkg-config --libs gtk+-3.0` \
			`pkg-config --libs cairo` \
			`pkg-config --libs libxml-2.0` \
			`pkg-config --libs gsoap`