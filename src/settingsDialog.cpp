/***********************************************************************
* SPDX-FileCopyrightText: 2003-2004 Max Howell <max.howell@methylblue.com>
* SPDX-FileCopyrightText: 2008-2009 Martin Sandsmark <martin.sandsmark@kde.org>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
***********************************************************************/

#include "settingsDialog.h"

#include <KLocalizedString>
#include <QRadioButton>
#include <QDir>
#include <KMessageBox>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QButtonGroup>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    QPushButton *resetButton = buttons->addButton(QDialogButtonBox::Reset);
    QPushButton *closeButton = buttons->addButton(QDialogButtonBox::Close);
    layout()->addWidget(buttons);

    m_schemaGroup = new QButtonGroup(this);
    QRadioButton *radioButton;

    radioButton = new QRadioButton(i18n("Rainbow"), this);
    colorSchemeLayout->addWidget(radioButton);
    m_schemaGroup->addButton(radioButton, Filelight::Rainbow);

    radioButton = new QRadioButton(i18n("System colors"), this);
    colorSchemeLayout->addWidget(radioButton);
    m_schemaGroup->addButton(radioButton, Filelight::KDE);

    radioButton = new QRadioButton(i18n("High contrast"), this);
    colorSchemeLayout->addWidget(radioButton);
    m_schemaGroup->addButton(radioButton, Filelight::HighContrast);

    //read in settings before you make all those nasty connections!
    reset(); //makes dialog reflect global settings

    connect(&m_timer, &QTimer::timeout, this, &SettingsDialog::mapIsInvalid);

    connect(m_addButton, &QPushButton::clicked, this, &SettingsDialog::addFolder);
    connect(m_removeButton, &QPushButton::clicked, this, &SettingsDialog::removeFolder);
    connect(resetButton, &QPushButton::clicked, this, &SettingsDialog::reset);
    connect(closeButton, &QPushButton::clicked, this, &SettingsDialog::close);
    connect(m_schemaGroup, static_cast<void (QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked), this, &SettingsDialog::changeScheme);

    connect(contrastSlider, &QSlider::valueChanged, this, &SettingsDialog::changeContrast);

    connect(scanAcrossMounts, &QCheckBox::toggled, this, &SettingsDialog::startTimer);
    connect(dontScanRemoteMounts, &QCheckBox::toggled, this, &SettingsDialog::startTimer);
    connect(scanAcrossMounts, &QCheckBox::toggled, this, &SettingsDialog::toggleScanAcrossMounts);
    connect(dontScanRemoteMounts, &QCheckBox::toggled, this, &SettingsDialog::toggleDontScanRemoteMounts);

    connect(useAntialiasing, &QCheckBox::toggled, this, &SettingsDialog::toggleUseAntialiasing);
    connect(varyLabelFontSizes, &QCheckBox::toggled, this, &SettingsDialog::toggleVaryLabelFontSizes);
    connect(showSmallFiles, &QCheckBox::toggled, this, &SettingsDialog::toggleShowSmallFiles);

    connect(minFontPitch, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::changeMinFontPitch);

    m_addButton->setIcon(QIcon::fromTheme(QStringLiteral("folder-open")));
    m_removeButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
}


void SettingsDialog::closeEvent(QCloseEvent*)
{
    //if an invalidation is pending, force it now!
    if (m_timer.isActive()) m_timer.setInterval(0);

    Config::write();

    deleteLater();
}


void SettingsDialog::reset()
{
    Config::read();

    //tab 1
    scanAcrossMounts->setChecked(Config::scanAcrossMounts);
    dontScanRemoteMounts->setChecked(!Config::scanRemoteMounts);

    dontScanRemoteMounts->setEnabled(Config::scanAcrossMounts);

    m_listBox->clear();
    m_listBox->addItems(Config::skipList);
    m_listBox->setCurrentRow(0);

    m_removeButton->setEnabled(m_listBox->count() > 0);

    //tab 2
    if (m_schemaGroup->checkedId() != Config::scheme) //TODO: This is probably wrong
    {
        m_schemaGroup->button(Config::scheme)->setChecked(true);
        //colourSchemeGroup->setSelected(Config::scheme);
        //setButton doesn't call a single QButtonGroup signal!
        //so we need to call this ourselves (and hence the detection above)
//        changeScheme(Config::scheme);
    }
    contrastSlider->setValue(Config::contrast);

    useAntialiasing->setChecked(Config::antialias);

    varyLabelFontSizes->setChecked(Config::varyLabelFontSizes);
    minFontPitchLabel->setEnabled(Config::varyLabelFontSizes);
    minFontPitch->setEnabled(Config::varyLabelFontSizes);
    minFontPitch->setValue(Config::minFontPitch);
    showSmallFiles->setChecked(Config::showSmallFiles);
}



void SettingsDialog::toggleScanAcrossMounts(bool b)
{
    Config::scanAcrossMounts = b;

    dontScanRemoteMounts->setEnabled(b);
}

void SettingsDialog::toggleDontScanRemoteMounts(bool b)
{
    Config::scanRemoteMounts = !b;
}



void SettingsDialog::addFolder()
{
    const QString urlString = QFileDialog::getExistingDirectory(this, i18n("Select path to ignore"), QDir::rootPath());
    const QUrl url = QUrl::fromLocalFile(urlString);

    //TODO error handling!
    //TODO wrong protocol handling!

    if (!url.isEmpty())
    {
        const QString path = url.toLocalFile();

        if (!Config::skipList.contains(path))
        {
            Config::skipList.append(path);
            m_listBox->addItem(path);
            if (m_listBox->currentItem() == nullptr) m_listBox->setCurrentRow(0);
            m_removeButton->setEnabled(true);
        }
        else KMessageBox::information(this, i18n("That folder is already set to be excluded from scans."), i18n("Folder already ignored"));
    }
}


void SettingsDialog::removeFolder()
{
    Config::skipList.removeAll(m_listBox->currentItem()->text()); //removes all entries that match

    //safest method to ensure consistency
    m_listBox->clear();
    m_listBox->addItems(Config::skipList);

    m_removeButton->setEnabled(m_listBox->count() > 0);
    if (m_listBox->count() > 0) m_listBox->setCurrentRow(0);
}


void SettingsDialog::startTimer()
{
    m_timer.setSingleShot(true);
    m_timer.start(TIMEOUT);
}

void SettingsDialog::changeScheme(QAbstractButton *button)
{
    if (button) {
        const int s = m_schemaGroup->id(button);
        Config::scheme = (Filelight::MapScheme)s;
        Q_EMIT canvasIsDirty(Dirty::Colors);
    }
}
void SettingsDialog::changeContrast(int c)
{
    Config::contrast = c;
    Q_EMIT canvasIsDirty(Dirty::Colors);
}
void SettingsDialog::toggleUseAntialiasing(bool b)
{
    Config::antialias = b;
    Q_EMIT canvasIsDirty(Dirty::AntiAliasing);
}
void SettingsDialog::toggleVaryLabelFontSizes(bool b)
{
    Config::varyLabelFontSizes = b;
    minFontPitchLabel->setEnabled(b);
    minFontPitch->setEnabled(b);
    Q_EMIT canvasIsDirty(Dirty::Font);
}
void SettingsDialog::changeMinFontPitch(int p)
{
    Config::minFontPitch = p;
    Q_EMIT canvasIsDirty(Dirty::Font);
}
void SettingsDialog::toggleShowSmallFiles(bool b)
{
    Config::showSmallFiles = b;
    Q_EMIT canvasIsDirty(Dirty::Layout);
}

void SettingsDialog::reject()
{
    //called when escape is pressed
    reset();
    QDialog::reject();   //**** doesn't change back scheme so far
}


