/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
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
#include <QUrl>

#include <QLabel>

using KParts::StatusBarExtension;
namespace RadialMap {
class Widget;
}
class Folder;


namespace Filelight
{
class Part;

class BrowserExtension : public KParts::BrowserExtension
{
public:
    explicit BrowserExtension(Part*);
};


class Part : public KParts::ReadOnlyPart
{
    Q_OBJECT

public:
    Part(QWidget *, QObject *, const QList<QVariant>&);

    virtual bool openFile() {
        return false;    //pure virtual in base class
    }
    virtual bool closeUrl();

    QString prettyUrl() const {
        return url().isLocalFile() ? url().toLocalFile() : url().toString();
    }

public slots:
    virtual bool openUrl(const QUrl&);
    void configFilelight();
    void rescan();

private slots:
    void postInit();
    void scanCompleted(Folder*);
    void mapChanged(const Folder*);

private:
    QStatusBar *statusBar() {
        return m_statusbar->statusBar();
    }
    void showSummary();

    QLayout            *m_layout;
    QWidget            *m_summary;
    BrowserExtension   *m_ext;
    StatusBarExtension *m_statusbar;
    RadialMap::Widget  *m_map;
    QWidget            *m_stateWidget;
    class ScanManager  *m_manager;
    QLabel             *m_numberOfFiles;

    bool m_started;

private:
    bool start(const QUrl&);

private slots:
    void updateURL(const QUrl &);
};
}

#endif
