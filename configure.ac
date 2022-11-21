AC_INIT([helloworld], [0.1], [maintainer@example.com])
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
AM_SILENT_RULES([yes])

# TODO handle missing package
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([GSOAP], [gsoap], [gsoap_pkg_local=;],
                  [gsoap_pkg_local="yes"])
PKG_CHECK_MODULES([GSOAPSSL], [gsoapssl], [gsoapssl_pkg_local=;],
                  [gsoapssl_pkg_local="yes"])
# if test "x$gsoap_pkg_local" != "xyes" || test "x$gsoapssl_pkg_local" != "xyes" ; then
#     AX_SUBDIRS_CONFIGURE([gsoap-2.8],[--with-openssl=/usr/lib/ssl])
# fi

# AC_ARG_VAR(GSOAP_SRC_DIR, [gsoap src directory])
AC_SUBST(GSOAP_SRC_DIR, '$(abs_srcdir)/subprojects/gsoap-2.8')
AC_SUBST(CERBERO_PKG, '$(abs_srcdir)/subprojects/cerbero/build/dist/linux_x86_64/lib/pkgconfig')
AC_CONFIG_SUBDIRS([
    OnvifDiscoveryLib
    OnvifSoapLib
])
LT_INIT([shared])
AC_CONFIG_FILES([
    OnvifDiscoveryLib/Makefile
    OnvifSoapLib/Makefile
    Makefile
])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_OUTPUT