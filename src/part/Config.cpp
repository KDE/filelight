
#include "Config.h"
#include <kconfig.h>
#include <kglobal.h>


bool Config::scanAcrossMounts;
bool Config::scanRemoteMounts;
bool Config::scanRemovableMedia;
bool Config::varyLabelFontSizes;
bool Config::showSmallFiles;
uint Config::contrast;
uint Config::antiAliasFactor;
uint Config::minFontPitch;
uint Config::defaultRingDepth;
Filelight::MapScheme Config::scheme;
QStringList Config::skipList;


inline KConfig&
Filelight::Config::kconfig()
{
    KConfig *config = KGlobal::config();
    config->setGroup( "filelight_part" );
    return *config;
}

void
Filelight::Config::read()
{
    const KConfig &config = kconfig();

    scanAcrossMounts   = config.readBoolEntry( "scanAcrossMounts", false );
    scanRemoteMounts   = config.readBoolEntry( "scanRemoteMounts", false );
    scanRemovableMedia = config.readBoolEntry( "scanRemovableMedia", false );
    varyLabelFontSizes = config.readBoolEntry( "varyLabelFontSizes", true );
    showSmallFiles     = config.readBoolEntry( "showSmallFiles", false );
    contrast           = config.readNumEntry( "contrast", 75 );
    antiAliasFactor    = config.readNumEntry( "antiAliasFactor", 2 );
    minFontPitch       = config.readNumEntry( "minFontPitch", QFont().pointSize() - 3);
    scheme = (MapScheme) config.readNumEntry( "scheme", 0 );
    skipList           = config.readPathListEntry( "skipList" );

    defaultRingDepth   = 4;
}

void
Filelight::Config::write()
{
    KConfig &config = kconfig();

    config.writeEntry( "scanAcrossMounts", scanAcrossMounts );
    config.writeEntry( "scanRemoteMounts", scanRemoteMounts );
    config.writeEntry( "scanRemovableMedia", scanRemovableMedia );
    config.writeEntry( "varyLabelFontSizes", varyLabelFontSizes );
    config.writeEntry( "showSmallFiles", showSmallFiles);
    config.writeEntry( "contrast", contrast );
    config.writeEntry( "antiAliasFactor", antiAliasFactor );
    config.writeEntry( "minFontPitch", minFontPitch );
    config.writeEntry( "scheme", scheme );
    config.writePathEntry( "skipList", skipList );
}
