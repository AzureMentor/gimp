## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
gfig_RC = gfig.rc.o
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = gfig-examples images

libexecdir = $(gimpplugindir)/plug-ins/gfig

libexec_PROGRAMS = gfig

gfig_SOURCES = \
	gfig.c			\
	gfig.h			\
	gfig-arc.c		\
	gfig-arc.h		\
	gfig-bezier.c		\
	gfig-bezier.h		\
	gfig-circle.c		\
	gfig-circle.h		\
	gfig-dialog.c		\
	gfig-dialog.h		\
	gfig-dobject.c		\
	gfig-dobject.h		\
	gfig-ellipse.c		\
	gfig-ellipse.h		\
	gfig-grid.c		\
	gfig-grid.h		\
	gfig-icons.c		\
	gfig-icons.h		\
	gfig-line.c		\
	gfig-line.h		\
	gfig-poly.c		\
	gfig-poly.h		\
	gfig-preview.c		\
	gfig-preview.h		\
	gfig-rectangle.c	\
	gfig-rectangle.h	\
	gfig-spiral.c		\
	gfig-spiral.h		\
	gfig-star.c		\
	gfig-star.h		\
	gfig-style.c		\
	gfig-style.h    	\
	gfig-types.h

EXTRA_DIST = README

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS) \
	-I$(includedir)

LDADD = \
	$(libm)			\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(gfig_RC)
