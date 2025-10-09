m4_define([onvifmgr_major_version], [0])
m4_define([onvifmgr_minor_version], [0])
...
m4_define([onvifmgr_version],[onvifmgr_major_version.onvifmgr_minor_version])

AC_INIT([onvifmgr], [onvifmgr_version], [maintainer@example.com])
AC_SUBST(APP_VERSION, [onvifmgr_version])
AC_SUBST(APP_VERSION_MAJ, [onvifmgr_major_version])
AC_SUBST(APP_VERSION_MIN, [onvifmgr_minor_version])

AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
AM_SILENT_RULES([yes])

if test "$srcdir" = "."; then
	AC_MSG_NOTICE([building in place srcdir:$srcdir])
	AC_DEFINE([BUILDING_IN_PLACE], 1, [building in place])
else
	AC_MSG_NOTICE([building out of tree srcdir:$srcdir])
fi

# TODO handle missing package
# gtk+-3.0 #libgtk-3-dev

AC_SUBST(GSOAP_SRC_DIR, [$srcdir/subprojects/gsoap-2.8])

AC_SUBST(PKG_GLIB_PATH, [$srcdir/subprojects/glib-2.85.2/dist/lib/pkgconfig])
AC_SUBST(PKG_PULSE_PATH, [$srcdir/subprojects/pulseaudio/build/dist/lib/pkgconfig])
AC_SUBST(PKG_ALSA_PATH, [$srcdir/subprojects/alsa-lib/build/dist/lib/pkgconfig])
AC_SUBST(PKG_GUDEV_PATH, [$srcdir/subprojects/libgudev/build/dist/lib/pkgconfig])
AC_SUBST(PKG_UDEV_PATH, [$srcdir/subprojects/systemd-256/build/dist/lib/pkgconfig])
AC_SUBST(PKG_FFMPEG_PATH, [$srcdir/subprojects/FFmpeg/dist/lib/pkgconfig])
#AC_SUBST(PKG_V4L2_PATH, [$srcdir/subprojects/v4l-utils/dist/lib/pkgconfig])
AC_SUBST(PKG_OPENSSL_PATH, [$srcdir/subprojects/openssl/build/dist/lib/pkgconfig])
AC_SUBST(PKG_ONVIF_DISCO_PATH, [$srcdir/subprojects/OnvifDiscoveryLib/build/dist/lib/pkgconfig])
AC_SUBST(PKG_ONVIF_SOAP_PATH, [$srcdir/subprojects/OnvifSoapLib/build/dist/lib/pkgconfig])
AC_SUBST(PKG_CUTILS_PATH, [$srcdir/subprojects/CUtils/build/dist/lib/pkgconfig])
AC_SUBST(PKG_DE265_PATH, [$srcdir/subprojects/libde265/dist/lib/pkgconfig])
AC_SUBST(PKG_NTLM_PATH, [$srcdir/subprojects/libntlm-1.8/build/dist/lib/pkgconfig])
AC_SUBST(PKG_LIBX11_PATH, [$srcdir/subprojects/libX11-1.8.10/build/dist/lib/pkgconfig])
AC_SUBST(PKG_GTK_PATH, [$srcdir/subprojects/gtk/dist/lib/pkgconfig])
#Declare all path for pkg-config to search
AC_SUBST(PKG_FULL_PATH, [$PKG_FFMPEG_PATH:$PKG_PULSE_PATH:$PKG_ALSA_PATH:$PKG_GUDEV_PATH:$PKG_UDEV_PATH:$PKG_GLIB_PATH:$PKG_V4L2_PATH:$PKG_ONVIF_DISCO_PATH:$PKG_ONVIF_SOAP_PATH:$PKG_CUTILS_PATH:$PKG_OPENSSL_PATH:$PKG_DE265_PATH:$PKG_NTLM_PATH:$PKG_LIBX11_PATH:$PKG_GTK_PATH])

#Gstreamer core packages normalized
AC_SUBST(GST_LIBS,m4_normalize('
    gstreamer-1.0
    gstreamer-video-1.0
    alsa
'))

#Gstreamer plugin packages noramlized
AC_SUBST(GST_PLUGIN_PACKAGES,m4_normalize('
    gstopengl
    gstgtk
    gstinterlace
    gstde265
    gstjpeg
    gstjpegformat
    gstlevel
    gstximagesink
    gstoverlaycomposition
    gstpulseaudio
    gstopenh264
    gstfdkaac
    gstv4l2codecs 
    gstvideo4linux2
    gstvideoparsersbad
    gstvideoconvertscale
    gstrtponvif
    gstudp
    gstcoreelements
    gstrtsp
    gstrtpmanager 
    gstrtp
    gstalaw
    gstmulaw
    gstinterleave
    gstautodetect
    gstaudioparsers
    gstalsa
    gstvolume
    gstvideorate
    gsttypefindfunctions
    gsttcp
    gstrawparse
    gstplayback
    gstpbtypes
    gstaudiotestsrc
    gstaudioresample
    gstaudioconvert
    gstapp
    gstopus
    gstgtkwayland
'))

#Function to check gstreamer built under subproject. (.pc file exists for each plugins)
AC_DEFUN([CHECK_GSTREAMER_PACKAGES],[
    og_pkg_path=$PKG_CONFIG_PATH
    AC_SUBST([PKG_CONFIG_PATH], [$PKG_FULL_PATH:$srcdir/subprojects/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig:$srcdir/subprojects/gstreamer/build/dist/lib/pkgconfig])
    PKG_CHECK_MODULES(gstreamerstatic,[${GST_PLUGIN_PACKAGES}], [], [AC_MSG_FAILURE("subprojects not build properly. Please run autogen.sh",1)])
    AC_SUBST([PKG_CONFIG_PATH], [$og_pkg_path])
])

#Function to search gstreamer system libraries (plugins don't have a .pc file on archlinux)
AC_DEFUN([CHECK_GSTREAMER_LIBS],[
    og_libs=$LIBS
    og_ldflags=$LDFLAGS
    AC_SUBST([LDFLAGS], ['-L/usr/lib/gstreamer-1.0 -L/usr/local/lib/gstreamer-1.0']) #plugin subfolder isn't actually part of system path
    AC_SEARCH_LIBS([gst_plugin_alaw_register], [gstalaw],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}alaw "])
    AC_SEARCH_LIBS([gst_plugin_alsa_register], [gstalsa],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}alsa "])
    AC_SEARCH_LIBS([gst_plugin_app_register], [gstapp],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}app "])
    AC_SEARCH_LIBS([gst_plugin_audioconvert_register], [gstaudioconvert],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}audioconvert "])
    AC_SEARCH_LIBS([gst_plugin_audioparsers_register], [gstaudioparsers],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}audioparsers "])
    AC_SEARCH_LIBS([gst_plugin_audioresample_register], [gstaudioresample],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}audioresample "])
    AC_SEARCH_LIBS([gst_plugin_audiotestsrc_register], [gstaudiotestsrc],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}audiotestsrc "])
    AC_SEARCH_LIBS([gst_plugin_autodetect_register], [gstautodetect],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}autodetect "])
    AC_SEARCH_LIBS([gst_plugin_coreelements_register], [gstcoreelements],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}coreelements "])
    #AC_SEARCH_LIBS([gst_plugin_de265_register], [gstde265],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}de265 "])
    AC_SEARCH_LIBS([gst_plugin_fdkaac_register], [gstfdkaac],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}fdkaac "])
    AC_SEARCH_LIBS([gst_plugin_gtk_register], [gstgtk],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}gtk "])
    AC_SEARCH_LIBS([gst_plugin_interlace_register], [gstinterlace],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}interlace "])
    AC_SEARCH_LIBS([gst_plugin_interleave_register], [gstinterleave],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}interleave "])
    AC_SEARCH_LIBS([gst_plugin_jpeg_register], [gstjpeg],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}jpeg "])
    AC_SEARCH_LIBS([gst_plugin_jpegformat_register], [gstjpegformat],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}jpegformat "])
    AC_SEARCH_LIBS([gst_plugin_level_register], [gstlevel],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}level "])
    AC_SEARCH_LIBS([gst_plugin_mulaw_register], [gstmulaw],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}mulaw "])
    AC_SEARCH_LIBS([gst_plugin_opengl_register], [gstopengl],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}opengl "])
    #AC_SEARCH_LIBS([gst_plugin_openh264_register], [gstopenh264],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}openh264 "])
    AC_SEARCH_LIBS([gst_plugin_overlaycomposition_register], [gstoverlaycomposition],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}overlaycomposition "])
    AC_SEARCH_LIBS([gst_plugin_pbtypes_register], [gstpbtypes],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}pbtypes "])
    AC_SEARCH_LIBS([gst_plugin_playback_register], [gstplayback],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}playback "])
    AC_SEARCH_LIBS([gst_plugin_pulseaudio_register], [gstpulseaudio],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}pulseaudio "])
    AC_SEARCH_LIBS([gst_plugin_rawparse_register], [gstrawparse],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}rawparse "])
    AC_SEARCH_LIBS([gst_plugin_rtsp_register], [gstrtsp],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}rtsp "])
    AC_SEARCH_LIBS([gst_plugin_rtp_register], [gstrtp],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}rtp "])
    AC_SEARCH_LIBS([gst_plugin_rtpmanager_register], [gstrtpmanager],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}rtpmanager "])
    AC_SEARCH_LIBS([gst_plugin_rtponvif_register], [gstrtponvif],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}rtponvif "])
    AC_SEARCH_LIBS([gst_plugin_tcp_register], [gsttcp],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}tcp "])
    AC_SEARCH_LIBS([gst_plugin_typefindfunctions_register], [gsttypefindfunctions],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}typefindfunctions "])
    AC_SEARCH_LIBS([gst_plugin_udp_register], [gstudp],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}udp "])
    AC_SEARCH_LIBS([gst_plugin_v4l2codecs_register], [gstv4l2codecs],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}v4l2codecs "])
    AC_SEARCH_LIBS([gst_plugin_video4linux2_register], [gstvideo4linux2],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}video4linux2 "])
    AC_SEARCH_LIBS([gst_plugin_videoconvertscale_register], [gstvideoconvertscale],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}videoconvertscale "])
    AC_SEARCH_LIBS([gst_plugin_videoparsersbad_register], [gstvideoparsersbad],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}videoparsersbad "])
    AC_SEARCH_LIBS([gst_plugin_videorate_register], [gstvideorate],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}videorate "])
    AC_SEARCH_LIBS([gst_plugin_volume_register], [gstvolume],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}volume "])
    AC_SEARCH_LIBS([gst_plugin_ximagesink_register], [gstximagesink],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}ximagesink "])
    AC_SEARCH_LIBS([gst_plugin_opus_register], [gstopus],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}opus "])
    AC_SEARCH_LIBS([gst_plugin_gtkwayland_register], [gstwaylandsink],[], [GST_PLUGIN_MISSING="${GST_PLUGIN_MISSING}wayland "])

    AS_IF([test -z "$GST_PLUGIN_MISSING"],[],[AC_MSG_RESULT([WARNING: Gstreamer system library missing : $GST_PLUGIN_MISSING])])
    AC_SUBST([LDFLAGS], [$og_ldflags]) #Restore ldflags
    LIBS=$og_libs #We handle libs manually. Clear automatically populated libs
])

AC_DEFUN([SET_GST_STATIC_VARS],[
    #Check subproject library
    CHECK_GSTREAMER_PACKAGES

    AC_SUBST(EXT_PLGS, '$(EXT_PLGS_LIBAV) $(EXT_PLGS_NV)')

    #Declare additional plugins to link statically
    AC_MSG_NOTICE([Setting static Gstreamer path])
    AC_SUBST(GST_LINK_TYPE, [-Wl,-Bstatic])
    AC_SUBST(GST_STATIC_FLAG,[-DSTATIC_BUILD])
    AC_SUBST(PKG_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/pkgconfig')
    AC_SUBST(PKG_GST_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig')
    AC_SUBST(GST_PLGS,'$(GST_PLUGIN_PACKAGES)') 
])

AC_ARG_ENABLE(libav,
    [AS_HELP_STRING([--enable-libav],[Enable libav gstreamer plugin. @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetlibav=yes ;;
        no)  targetlibav=no ;;
        true) targetlibav=yes ;;
        false)  targetlibav=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-libav]) ;;
        esac],
    [targetlibav="no"])
AM_CONDITIONAL([LIBAVCOND], [test x$targetlibav = xyes])
AM_COND_IF([LIBAVCOND],[
    AC_SUBST(EXT_PLGS_LIBAV, gstlibav)
    AC_SUBST(EXT_PKG_LIBAV, "gstlibav >= 1.14.4")
    AC_SUBST(EXT_CFLAGS_LIBAV, -DENABLELIBAV)
    AC_MSG_RESULT([configure: checking libav enabled... yes]) 
],[
    AC_MSG_RESULT([configure: checking libav enabled... no]) 
])

AC_ARG_ENABLE(nvcodec,
    [AS_HELP_STRING([--enable-nvcodec],[Enable Nvidia codec gstreamer plugin. @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetnvcodec=yes ;;
        no)  targetnvcodec=no ;;
        true) targetnvcodec=yes ;;
        false)  targetnvcodec=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-nvcodec]) ;;
        esac],
        [targetnvcodec="no"])
AM_CONDITIONAL([NVCODECCOND], [test x$targetnvcodec = xyes])
AM_COND_IF([NVCODECCOND],[
    AC_SUBST(EXT_PLGS_NV, "gstnvcodec")
    AC_SUBST(EXT_PKG_NV, "gstnvcodec >= 1.14.4")
    AC_SUBST(EXT_CFLAGS_NV, -DENABLENVCODEC)
    AC_MSG_RESULT([configure: checking NVIDIA codec enabled... yes]) 
],[
    AC_MSG_RESULT([configure: checking NVIDIA codec enabled... no])
])

# "gstreamer-plugins-good-1.0 >= 1.14.4" apparently doesn't exist on MX Linux?
AC_SUBST(GST_PKG_CORE,m4_normalize(
    "gstreamer-1.0 >= 1.14.4 
    gstreamer-video-1.0 >= 1.14.4
    gstreamer-plugins-base-1.0 >= 1.14.4 
    gstreamer-plugins-bad-1.0 >= 1.14.4"
))

#Check for debug flag
AC_ARG_ENABLE(debug,
    [AS_HELP_STRING([--enable-debug],[Enable runtime debugging @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetdebug=yes ;;
        no)  targetdebug=no ;;
        true) targetdebug=yes ;;
        false)  targetdebug=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
        esac],
    [targetdebug="no"])
AM_CONDITIONAL([DEBUGCOND], [test x$targetdebug = xyes])
AM_COND_IF([DEBUGCOND],[
    AC_MSG_RESULT([DEBUGING MODE enabled])
    AC_SUBST(DEBUG_FLAG, '-g -O0')
],[
    AC_MSG_RESULT([DEBUGING MODE disabled])
])

#Check if gstreamer is installed
AC_ARG_ENABLE(latest,
    [AS_HELP_STRING([--enable-latest],[Force the usage or build gstreamer 1.21.90. @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetgstbuild=yes ;;
        no)  targetgstbuild=no ;;
        true) targetgstbuild=yes ;;
        false)  targetgstbuild=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-latest]) ;;
        esac],
    [targetgstbuild="no"])
AM_CONDITIONAL([GSTBUILDCOND], [test x$targetgstbuild = xyes])
AM_COND_IF([GSTBUILDCOND],[
    AC_MSG_NOTICE([gstreamer forced - Using latest static libraries])
    SET_GST_STATIC_VARS
], [
    PKG_CHECK_MODULES(gstreamer,[${GST_PKG_CORE}], [
        #Check system library
        CHECK_GSTREAMER_LIBS
        AS_IF([test -z "$GST_PLUGIN_MISSING"],[
            AC_MSG_NOTICE([gstreamer >= 1.14.4 found - Using shared libraries])
            AC_SUBST(GST_LINK_TYPE, [-Wl,-Bdynamic])
        ],[
            AC_MSG_NOTICE([One or more gstreamer plugin missing - Using latest static libraries])
            SET_GST_STATIC_VARS
        ])
    ], [
        AC_MSG_NOTICE([gstreamer >= 1.14.4 not found. - Using latest static libraries])
        SET_GST_STATIC_VARS
    ])
])

AC_SUBST(EXT_CFLAGS, '$(EXT_CFLAGS_LIBAV) $(EXT_CFLAGS_NV)')

#Declare all path for pkg-config to search
AC_SUBST(PKG_FULL_PATH, [$PKG_PATH:$PKG_GST_PATH:$PKG_FFMPEG_PATH:$PKG_PULSE_PATH:$PKG_ALSA_PATH:$PKG_GUDEV_PATH:$PKG_UDEV_PATH:$PKG_GLIB_PATH:$PKG_V4L2_PATH:$PKG_ONVIF_DISCO_PATH:$PKG_ONVIF_SOAP_PATH:$PKG_CUTILS_PATH:$PKG_OPENSSL_PATH:$PKG_DE265_PATH:$PKG_NTLM_PATH:$PKG_LIBX11_PATH:$PKG_GTK_PATH])


AC_CONFIG_FILES([
    Makefile
])

LT_INIT([shared])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_OUTPUT