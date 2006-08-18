SUBDIRS     = radialMap
INCLUDES    = $(all_includes) -I$(top_srcdir)/filelight/src
METASOURCES = AUTO

#Part
kde_module_LTLIBRARIES  = libfilelight.la
libfilelight_la_LIBADD  = ./radialMap/libradialmap.la $(LIB_KFILE) $(LIB_KPARTS) $(LIB_KDEUI) $(LIB_QT)
libfilelight_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libfilelight_la_SOURCES = \
   dialog.ui \
   part.cpp \
   scan.cpp \
   progressBox.cpp \
   Config.cpp \
   settingsDialog.cpp \
   fileTree.cpp \
   localLister.cpp \
   remoteLister.cpp \
   summaryWidget.cpp
