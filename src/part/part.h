/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <sandsmark@samfundet.no>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#ifndef FILELIGHTPART_H
#define FILELIGHTPART_H

#include <KParts/BrowserExtension>
#include <KParts/StatusBarExtension>
#include <KParts/Part>
#include <KUrl>

class KAboutData;
using KParts::StatusBarExtension;
namespace RadialMap {
class Widget;
}
class Directory;


namespace Filelight
{
class Part;

class BrowserExtension : public KParts::BrowserExtension
{
public:
    BrowserExtension(Part*);
};


class Part : public KParts::ReadOnlyPart
{
    Q_OBJECT

public:
    Part(QWidget *, QObject *, const QList<QVariant>&);

    virtual bool openFile() {
        return false;    //pure virtual in base class
    }
    virtual bool closeURL();

    QString prettyUrl() const {
        return url().protocol() == "file" ? url().path() : url().prettyUrl();
    }

    static KAboutData *createAboutData();

public slots:
    virtual bool openURL(const KUrl&);
    void configFilelight();
    void rescan();

private slots:
    void postInit();
    void scanCompleted(Directory*);
    void mapChanged(const Directory*);

private:
    KStatusBar *statusBar() {
        return m_statusbar->statusBar();
    }

    QLayout            *m_layout;
    BrowserExtension   *m_ext;
    StatusBarExtension *m_statusbar;
    RadialMap::Widget  *m_map;
    class ScanManager  *m_manager;

    bool m_started;

private:
    bool start(const KUrl&);

private slots:
    void updateURL(const KUrl &);
};
}

#endif
