#!bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h` -o $podir/APPNAME.pot

