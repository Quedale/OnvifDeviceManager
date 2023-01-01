AC_INIT([helloworld], [0.1], [maintainer@example.com])
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
AM_SILENT_RULES([yes])

# TODO handle missing package

AC_ARG_ENABLE(libav,
    [AS_HELP_STRING([--enable-rpi],[Enable libav gstreamer plugin. @<:@default=no@:>@.])],
    [case "${enableval}" in
        yes) targetlibav=yes ;;
        no)  targetlibav=no ;;
        true) targetlibav=yes ;;
        false)  targetlibav=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
        esac],
    [targetlibav="no"])
AM_CONDITIONAL([LIBAVCOND], [test x$targetlibav = xyes])
AM_COND_IF([LIBAVCOND],[
    AC_SUBST(EXT_PLGS, gstlibav)
    AC_SUBST(EXT_CFLAGS, -DENABLELIBAV)
])

AC_SUBST(GSOAP_SRC_DIR, '$(abs_srcdir)/subprojects/gsoap-2.8')
AC_SUBST(PKG_PULSE_PATH, '$(abs_srcdir)/subprojects/pulseaudio/build/dist/lib/pkgconfig')
AC_SUBST(PKG_ALSA_PATH, '$(abs_srcdir)/subprojects/alsa-lib/build/dist/lib/pkgconfig')
AC_SUBST(PKG_GUDEV_PATH, '$(abs_srcdir)/subprojects/libgudev/build/dist/lib/pkgconfig')
AC_SUBST(PKG_UDEV_PATH, '$(abs_srcdir)/subprojects/systemd/build/dist/usr/lib/pkgconfig')
AC_SUBST(LIB_UDEV_PATH, '-L$(abs_srcdir)/subprojects/systemd/build/dist/usr/lib')
AC_SUBST(PKG_FFMPEG_PATH, '$(abs_srcdir)/subprojects/FFmpeg/dist/lib/pkgconfig')

AC_SUBST(PKG_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/pkgconfig')
AC_SUBST(PKG_GST_PATH, '$(abs_srcdir)/subprojects/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig')
AC_SUBST(PKG_GSTLIBAV_PATH, '$(abs_srcdir)/subprojects/gstreamer/libav_build/dist/lib/gstreamer-1.0/pkgconfig:$(abs_srcdir)/subprojects/gstreamer/libav_build/dist/lib/pkgconfig')
AC_SUBST(PKG_FULL_PATH, '$(PKG_PATH):$(PKG_GST_PATH):$(PKG_GSTLIBAV_PATH):$(PKG_FFMPEG_PATH):$(PKG_PULSE_PATH):$(PKG_ALSA_PATH):$(PKG_GUDEV_PATH):$(PKG_UDEV_PATH)')

AC_CONFIG_SUBDIRS([
    subprojects/OnvifDiscoveryLib
    subprojects/OnvifSoapLib
])

AC_CONFIG_FILES([
    subprojects/OnvifDiscoveryLib/Makefile
    subprojects/OnvifSoapLib/Makefile
    Makefile
])

LT_INIT([shared])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_OUTPUT