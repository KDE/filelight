#!bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h -o -name \*.qml` -o $podir/filelight.pot
rm -f rc.cpp
