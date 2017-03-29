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

#include <QUrl>

#include <KXmlGuiWindow>

class KJob;
class QLabel;

namespace RadialMap {
class Widget;
}
class Folder;


namespace Filelight
{
class Part;
class SummaryWidget;

class Part : public
//        KParts::ReadOnlyPart
        KXmlGuiWindow
{
    Q_OBJECT

public:
    Part(QWidget *, QObject *, const QList<QVariant>&);

    virtual bool openFile();
    virtual bool closeUrl();

    QString prettyUrl() const;

signals:
    void started(KJob *);
    void completed();
    void canceled(QString);
    void setWindowCaption(QString);

public slots:
    virtual bool openUrl(const QUrl&);
    void configFilelight();
    void rescan();

private slots:
    void postInit();
    void scanCompleted(Folder*);
    void mapChanged(const Folder*);

private:
    void showSummary();

    QLayout            *m_layout;
    SummaryWidget      *m_summary;
    RadialMap::Widget  *m_map;
    QWidget            *m_stateWidget;
    class ScanManager  *m_manager;
    QLabel             *m_numberOfFiles;

    bool m_started;

private:
    bool start(const QUrl&);

private slots:
    void updateURL(const QUrl &);


    // Compat
public:
    QUrl url() const;
    QWidget *widget() const;
private:
    void setUrl(const QUrl &url);
    void setWidget(QWidget *widget);
    QWidget *m_widget = nullptr;
    QUrl m_url;
};

}

#endif
