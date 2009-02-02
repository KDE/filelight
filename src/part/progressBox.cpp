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

#include "scan.h"
#include "progressBox.h"

#include <KGlobal>
#include <KGlobalSettings>
#include <KIO/Job>
#include <KLocale>

#include <QLabel>


ProgressBox::ProgressBox(QWidget *parent, QObject *part)
        : QLabel(parent)
{
    hide();

    setObjectName("ProgressBox");

    setAlignment(Qt::AlignCenter);
    setFont(KGlobalSettings::fixedFont());
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    setText(999999);
    setMinimumWidth(sizeHint().width());

    connect(&m_timer, SIGNAL(timeout()), SLOT(report()));
    connect(part, SIGNAL(started(KIO::Job*)), SLOT(start()));
    connect(part, SIGNAL(completed()), SLOT(stop()));
    connect(part, SIGNAL(canceled(const QString&)), SLOT(halt()));
}

void
ProgressBox::start() //slot
{
    m_timer.start(50); //20 times per second - very smooth
    report();
    show();
}

void
ProgressBox::report() //slot
{
    setText(Filelight::ScanManager::files());
}

void
ProgressBox::stop()
{
    m_timer.stop();
}

void
ProgressBox::halt()
{
    // canceled by stop button
    m_timer.stop();
    QTimer::singleShot(2000, this, SLOT(hide()));
}

void
ProgressBox::setText(int files)
{
    QLabel::setText(i18np("%1 File", "%1 Files", files));
}

#include "progressBox.moc"
