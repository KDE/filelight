SUBDIRS = kradialmap
INCLUDES = $(all_includes)
METASOURCES = AUTO

EXTRA_DIST = filelight.h scanbox.h \
             settingsdialog.ui settingsdlg.h filetree.h \
             scanmanager.h settings.h

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.cpp \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) rc.cpp $$LIST -o $(podir)/filelight.pot; \
	fi

KDE_ICON = open filelight

rcdir = $(kde_datadir)/filelight
rc_DATA = filelight_partui.rc filelightui.rc

# Application

bin_PROGRAMS = filelight

filelight_SOURCES = main.cpp filelight.cpp filelightpart.cpp scanmanager.cpp settings.cpp settingsdialog.ui settingsdlg.cpp scanbox.cpp historyaction.cpp
filelight_LDADD   = ./kradialmap/libkradialmap.a $(LIB_KFILE) $(LIB_KPARTS) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)
filelight_LDFLAGS = $(all_libraries) $(KDE_RPATH)

xdg_apps_DATA = filelight.desktop

configdir = $(kde_confdir)
config_DATA = filelightrc


# The KPart

kde_module_LTLIBRARIES = libfilelight.la
libfilelight_la_SOURCES = filelightpart.cpp scanmanager.cpp settings.cpp settingsdialog.ui settingsdlg.cpp
libfilelight_la_LIBADD  = ./kradialmap/libkradialmap.a $(LIB_KFILE) $(LIB_KPARTS) $(LIB_KDEUI) $(LIB_QT)
libfilelight_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

partdesktopdir   = $(kde_servicesdir)
partdesktop_DATA = filelight_part.desktop
