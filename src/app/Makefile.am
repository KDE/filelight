INCLUDES = $(all_includes) -I$(top_srcdir)/filelight/src
METASOURCES = AUTO

# Application
bin_PROGRAMS = filelight
filelight_SOURCES = historyAction.cpp mainWindow.cpp main.cpp
filelight_LDADD = $(LIB_KPARTS) $(LIB_KDEUI)
filelight_LDFLAGS = $(all_libraries) $(KDE_RPATH)
