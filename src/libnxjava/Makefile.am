lib_LTLIBRARIES = libnxjava.la

libnxjava_la_SOURCES = config.cpp jre.cpp jvm.cpp main.cpp platform.cpp tools.cpp
libnxjava_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXJAVA_EXPORTS -I@top_srcdir@/build
libnxjava_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxjava_la_LIBADD = @top_srcdir@/src/libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxjava.vcxproj libnxjava.vcxproj.filters \
	libnxjava.h

if JAR_BUILD
SUBDIRS = java
else
SUBDIRS =
endif
