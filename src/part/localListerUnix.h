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

#ifndef LOCALLISTERUNIX_H
#define LOCALLISTERUNIX_H

#include "localLister.h"

#include <QtCore/QByteArray>

class Folder;

namespace Filelight
{
class ScanManager;

class LocalListerUnix : public LocalLister
{
    Q_OBJECT
public:
    using LocalLister::LocalLister;

protected:
    Folder *scan(const QByteArray&, const QByteArray&) Q_DECL_OVERRIDE Q_DECL_FINAL;
};
}

#endif // LOCALLISTERUNIX_H
