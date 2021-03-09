/***********************************************************************
* SPDX-FileCopyrightText: 2020 Shubham <aryan100jangid@gmail.com>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/ 

#ifndef TESTFILETREE_H
#define TESTFILETREE_H

#include "fileTree.h"

#include <QTest>

class TestFileTree : public QObject
{
    Q_OBJECT

public:
    TestFileTree();
   ~TestFileTree(); 
    
private:
    void testFileName();
    void testFileSize();
    void testFilePath();

private:
    File *fl;
};

#endif
