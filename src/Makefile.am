INCLUDES= $(all_includes)
METASOURCES = AUTO

EXTRA_DIST = filelight.h scanbox.h segmenttip.h filemap.h \
             settingsdialog.ui settingsdlg.h filetree.h \
             scanmanager.h settings.h canvas.h builder.h \
             historyaction.h define.h

bin_PROGRAMS = filelight

filelight_SOURCES = main.cpp utils.cpp builder.cpp settings.cpp \
                    canvas.cpp canvaslabels.cpp canvasevents.cpp \
                    filemap.cpp segmenttip.cpp \
                    settingsdlg.cpp settingsdialog.ui \
                    filelight.cpp scanmanager.cpp scanbox.cpp
filelight_LDADD   = $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)
filelight_LDFLAGS = $(all_libraries) $(KDE_RPATH)

KDE_ICON = open filelight

utilitiesdir = $(kde_appsdir)/Utilities
utilities_DATA = filelight.desktop

rcdir = $(kde_datadir)/filelight
rc_DATA = filelightui.rc

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) rc.cpp $$LIST -o $(podir)/filelight.pot; \
	fi
