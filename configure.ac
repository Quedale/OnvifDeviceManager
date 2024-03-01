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

# TODO handle missing package
# gtk+-3.0 #libgtk-3-dev

AC_SUBST(GSOAP_SRC_DIR, '$(abs_srcdir)/subprojects/gsoap-2.8')

AC_SUBST(PKG_GLIB_PATH, '$(abs_srcdir)/subprojects/glib-2.74.1/dist/lib/pkgconfig')
AC_SUBST(PKG_PULSE_PATH, '$(abs_srcdir)/subprojects/pulseaudio/build/dist/lib/pkgconfig')
AC_SUBST(PKG_ALSA_PATH, '$(abs_srcdir)/subprojects/alsa-lib/build/dist/lib/pkgconfig')
AC_SUBST(PKG_GUDEV_PATH, '$(abs_srcdir)/subprojects/libgudev/build/dist/lib/pkgconfig')
AC_SUBST(PKG_UDEV_PATH, '$(abs_srcdir)/subprojects/systemd-252/dist/usr/local/lib/pkgconfig')
AC_SUBST(LIB_UDEV_PATH, '-L$(abs_srcdir)/subprojects/systemd-252/dist/usr/lib')
AC_SUBST(PKG_FFMPEG_PATH, '$(abs_srcdir)/subprojects/FFmpeg/dist/lib/pkgconfig')
#AC_SUBST(PKG_V4L2_PATH, '$(abs_srcdir)/subprojects/v4l-utils/dist/lib/pkgconfig')
AC_SUBST(PKG_OPENSSL_PATH, '$(abs_srcdir)/subprojects/openssl/build/dist/lib/pkgconfig')
AC_SUBST(PKG_ONVIF_DISCO_PATH, '$(abs_srcdir)/subprojects/OnvifDiscoveryLib/build/dist/lib/pkgconfig')
AC_SUBST(PKG_ONVIF_SOAP_PATH, '$(abs_srcdir)/subprojects/OnvifSoapLib/build/dist/lib/pkgconfig')
AC_SUBST(PKG_CUTILS_PATH, '$(abs_srcdir)/subprojects/CUtils/build/dist/lib/pkgconfig')
AC_SUBST(PKG_DE265_PATH, '$(abs_srcdir)/subprojects/libde265/dist/lib/pkgconfig')

AC_SUBST(GST_LIBS,m4_normalize('
    gstreamer-1.0
    gstreamer-video-1.0
    alsa
'))

AC_DEFUN([SET_GST_PKG_PATH],[
    #Declare additional plugins to link statically
    AC_MSG_NOTICE([Setting static Gstreamer path])
    AC_SUBST(GST_LINK_TYPE, [-Wl,-Bstatic])
    AC_SUBST(GST_STATIC_FLAG,[-DSTATIC_BUILD])
    AC_SUBST(PKG_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/pkgconfig')
    AC_SUBST(PKG_GST_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig')
    AC_SUBST(GST_PLGS,m4_normalize('
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
    ')) 
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
    AC_MSG_NOTICE([DEBUGING MODE enabled])
    AC_SUBST(DEBUG_FLAG, '-g -O0')
],[
    AC_MSG_NOTICE([DEBUGING MODE disabled])
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
    AC_SUBST(EXT_PLGS, '$(EXT_PLGS_LIBAV) $(EXT_PLGS_NV)')
    SET_GST_PKG_PATH
], [
    PKG_CHECK_MODULES(gstreamer,[${GST_PKG_CORE}], [
        AC_MSG_NOTICE([gstreamer >= 1.14.4 found - Using shared libraries])
        AC_SUBST(GST_LINK_TYPE, [-Wl,-Bdynamic])
    ], [
        AC_MSG_NOTICE([gstreamer >= 1.14.4 not found. - Using latest static libraries])
        #Apply extra defined plugin only when using static library
        AC_SUBST(EXT_PLGS, '$(EXT_PLGS_LIBAV) $(EXT_PLGS_NV)')
        SET_GST_PKG_PATH
    ])
])

AC_SUBST(EXT_CFLAGS, '$(EXT_CFLAGS_LIBAV) $(EXT_CFLAGS_NV)')

#Declare all path for pkg-config to search
AC_SUBST(PKG_FULL_PATH, '$(PKG_PATH):$(PKG_GST_PATH):$(PKG_FFMPEG_PATH):$(PKG_PULSE_PATH):$(PKG_ALSA_PATH):$(PKG_GUDEV_PATH):$(PKG_UDEV_PATH):$(PKG_GLIB_PATH):$(PKG_V4L2_PATH):$(PKG_ONVIF_DISCO_PATH):$(PKG_ONVIF_SOAP_PATH):$(PKG_CUTILS_PATH):$(PKG_OPENSSL_PATH):$(PKG_DE265_PATH)')


AC_CONFIG_FILES([
    Makefile
])

LT_INIT([shared])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_OUTPUT