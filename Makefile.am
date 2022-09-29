AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = guitest soaptest
guitest_SOURCES = gui-test.c
soaptest_SOURCES = soap-test.c


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