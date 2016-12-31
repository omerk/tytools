/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#include <QDesktopServices>
#include <QFileDialog>
#include <QUrl>

#include "../tyqt/board.hpp"
#include "../tyqt/monitor.hpp"
#include "../tyqt/task.hpp"
#include "updater_window.hpp"
#include "upty.hpp"

using namespace std;

QVariant UpdaterWindowModelFilter::data(const QModelIndex &index, int role) const
{
    if (index.column() == Monitor::COLUMN_BOARD && role == Qt::DisplayRole) {
        auto board = Monitor::boardFromModel(this, index);
        return QString("%1 %2").arg(board->description()).arg(board->serialNumber());
    }

    return QIdentityProxyModel::data(index, role);
}

UpdaterWindow::UpdaterWindow(QWidget *parent)
    : QMainWindow(parent), monitor_(upTy->monitor())
{
    setupUi(this);
    setWindowTitle(QApplication::applicationName());

    if (QFile::exists(":/logo"))
        logoLabel->setPixmap(QPixmap(":/logo"));
    resize(0, 0);

    connect(actionUpload, &QAction::triggered, this, &UpdaterWindow::uploadNewToCurrent);
    connect(actionReset, &QAction::triggered, this, &UpdaterWindow::resetCurrent);
    connect(actionQuit, &QAction::triggered, this, &UpTy::quit);

    connect(actionOpenLog, &QAction::triggered, upTy, &UpTy::showLogWindow);
#ifdef TY_CONFIG_URL_WEBSITE
    connect(actionWebsite, &QAction::triggered, this, &UpdaterWindow::openWebsite);
#else
    actionWebsite->setVisible(false);
#endif
#ifdef TY_CONFIG_URL_BUGS
    connect(actionReportBug, &QAction::triggered, this, &UpdaterWindow::openBugReports);
#else
    actionReportBug->setVisible(false);
#endif

    connect(boardComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UpdaterWindow::currentChanged);
    monitor_model_.setSourceModel(monitor_);
    boardComboBox->setModel(&monitor_model_);
    connect(uploadButton, &QToolButton::clicked, this, &UpdaterWindow::uploadNewToCurrent);
    connect(resetButton, &QToolButton::clicked, this, &UpdaterWindow::resetCurrent);

    // Error messages
    connect(upTy, &UpTy::globalError, this, &UpdaterWindow::showErrorMessage);

    if (!current_board_)
        changeCurrentBoard(nullptr);
}

bool UpdaterWindow::event(QEvent *ev)
{
    if (ev->type() == QEvent::StatusTip)
        return true;

    return QMainWindow::event(ev);
}

void UpdaterWindow::showErrorMessage(const QString &msg)
{
    statusBar()->showMessage(msg, TY_SHOW_ERROR_TIMEOUT);
}

void UpdaterWindow::uploadNewToCurrent()
{
    if (!current_board_)
        return;

    auto filename = QFileDialog::getOpenFileName(this, tr("Select a firmware for this device"),
                                                 current_board_->firmware(), browseFirmwareFilter());
    if (filename.isEmpty())
        return;

    current_board_->startUpload(filename);
}

void UpdaterWindow::resetCurrent()
{
    if (!current_board_)
        return;

    current_board_->startReset();
}

#ifdef TY_CONFIG_URL_WEBSITE
    void openWebsite();
#endif
#ifdef TY_CONFIG_URL_BUGS
    void openBugReports();
#endif

#ifdef TY_CONFIG_URL_WEBSITE
void UpdaterWindow::openWebsite()
{
    QDesktopServices::openUrl(QUrl(TY_CONFIG_URL_WEBSITE));
}
#endif

#ifdef TY_CONFIG_URL_BUGS
void UpdaterWindow::openBugReports()
{
    QDesktopServices::openUrl(QUrl(TY_CONFIG_URL_BUGS));
}
#endif

void UpdaterWindow::changeCurrentBoard(Board *board)
{
    if (current_board_) {
        current_board_->disconnect(this);
        current_board_ = nullptr;
    }

    if (board) {
        current_board_ = board->shared_from_this();

        connect(board, &Board::interfacesChanged, this, &UpdaterWindow::refreshActions);
        connect(board, &Board::statusChanged, this, &UpdaterWindow::refreshActions);
        connect(board, &Board::statusChanged, this, &UpdaterWindow::refreshProgress);
        connect(board, &Board::progressChanged, this, &UpdaterWindow::refreshProgress);
    }

    refreshActions();
}

void UpdaterWindow::refreshActions()
{
    bool upload = false, reset = false;

    if (current_board_) {
        if (current_board_->taskStatus() == TY_TASK_STATUS_READY) {
            upload = current_board_->hasCapability(TY_BOARD_CAPABILITY_UPLOAD) ||
                     current_board_->hasCapability(TY_BOARD_CAPABILITY_REBOOT);
            reset = current_board_->hasCapability(TY_BOARD_CAPABILITY_RESET) ||
                    current_board_->hasCapability(TY_BOARD_CAPABILITY_REBOOT);
        }
    } else {
        stackedWidget->setCurrentIndex(0);
    }

    uploadButton->setEnabled(upload);
    actionUpload->setEnabled(upload);
    resetButton->setEnabled(reset);
    actionReset->setEnabled(reset);
}

void UpdaterWindow::refreshProgress()
{
    auto task = current_board_->task();

    if (task.status() == TY_TASK_STATUS_PENDING || task.status() == TY_TASK_STATUS_RUNNING) {
        stackedWidget->setCurrentIndex(1);
        taskProgress->setRange(0, task.progressMaximum());
        taskProgress->setValue(task.progress());
    } else {
        stackedWidget->setCurrentIndex(0);
    }
}

QString UpdaterWindow::browseFirmwareFilter() const
{
    QString exts;
    for (unsigned int i = 0; i < ty_firmware_formats_count; i++)
        exts += QString("*%1 ").arg(ty_firmware_formats[i].ext);
    exts.chop(1);

    return tr("Binary Files (%1);;All Files (*)").arg(exts);
}

void UpdaterWindow::currentChanged(int index)
{
    changeCurrentBoard(Monitor::boardFromModel(&monitor_model_, index).get());
}
