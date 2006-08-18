SUBDIRS = src misc

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui"` >> rc.cpp
	LIST=`find src -name \*.h -o -name \*.cpp` ;\
	if test -n "$$LIST"; then \
	  $(XGETTEXT) rc.cpp $$LIST -o $(podir)/filelight.pot; \
	fi
