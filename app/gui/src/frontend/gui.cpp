// Unit Include
#include "gui.h"

// Qt Includes
#include <QApplication>
#include <QWindow>
#include <QFileDialog>
#include <QInputDialog>

// Qx Includes
#include <qx/widgets/qx-common-widgets.h>

// Magic enum
#include "magic_enum_utility.hpp"

//===============================================================================================================
// FrontendGui
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------------
//Public:
FrontendGui::FrontendGui(QApplication* app) :
    FrontendFramework(app)
{
    app->setQuitOnLastWindowClosed(false);
#ifdef __linux__
    // Set application icon
    app->setWindowIcon(appIconFromResources());
#endif
    setupTrayIcon();
    setupProgressDialog();
}

//-Class Functions--------------------------------------------------------------------------------------------------------
//Private:
QMessageBox::StandardButtons FrontendGui::choicesToButtons(DBlockingError::Choices cs)
{
    QMessageBox::StandardButtons bs;

    magic_enum::enum_for_each<DBlockingError::Choice>([&] (auto val) {
        constexpr DBlockingError::Choice c = val;
        if(cs.testFlag(c))
            bs.setFlag(smChoiceButtonMap.toRight(c));
    });

    return bs;
}

template<class MessageT>
    requires Qx::any_of<MessageT, DMessage, DBlockingMessage>
QMessageBox* FrontendGui::prepareMessageBox(const MessageT& dMsg)
{
    QMessageBox* msg  = new QMessageBox();
    msg->setIcon(QMessageBox::Information);
    msg->setWindowTitle(QCoreApplication::applicationName()); // This should be the default, but hey being explicit never hurt
    msg->setText(dMsg.text);

    if(dMsg.selectable)
        msg->setTextInteractionFlags(Qt::TextSelectableByMouse);

    return msg;
}

bool FrontendGui::windowsAreOpen()
{
    // Based on Qt's own check here: https://code.qt.io/cgit/qt/qtbase.git/tree/src/gui/kernel/qwindow.cpp?h=5.15.2#n2710
    // and here: https://code.qt.io/cgit/qt/qtbase.git/tree/src/gui/kernel/qguiapplication.cpp?h=5.15.2#n3629
    QWindowList topWindows = QApplication::topLevelWindows();
    for(const QWindow* window : qAsConst(topWindows))
    {
        if (window->isVisible() && !window->transientParent() && window->type() != Qt::ToolTip)
            return true;
    }
    return false;
}

QIcon& FrontendGui::trayExitIconFromResources() { static QIcon ico(u":/frontend/tray/Exit.png"_s); return ico; }

//-Instance Functions------------------------------------------------------------------------------------------------------
//Private:
void FrontendGui::handleMessage(const DMessage& d)
{
    auto mb = prepareMessageBox(d);
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
}

void FrontendGui::handleError(const DError& d) { Qx::postError(d.error); }

void FrontendGui::handleProcedureStart(const DProcedureStart& d)
{
    // Set label
    mProgressDialog.setLabelText(d.label);

    // Show right away
    mProgressDialog.setValue(0);
}

void FrontendGui::handleProcedureStop(const DProcedureStop& d)
{
    Q_UNUSED(d);
    /* Always reset the dialog regardless of whether it is visible or not as it may not be currently visible,
     * but queued to be visible on the next event loop iteration and therefore still needs to be hidden
     * immediately after.
     */
    mProgressDialog.reset();
}

void FrontendGui::handleProcedureProgress(const DProcedureProgress& d) { mProgressDialog.setValue(d.current); }
void FrontendGui::handleProcedureScale(const DProcedureScale& d) { mProgressDialog.setMaximum(d.max); }
void FrontendGui::handleStatusUpdate(const DStatusUpdate& d) { mStatusHeading = d.heading; mStatusMessage = d.message; }

// Sync directive handlers
void FrontendGui::handleBlockingMessage(const DBlockingMessage& d)
{
    auto mb = prepareMessageBox(d);
    mb->exec();
    delete mb;
}

void FrontendGui::handleBlockingError(const DBlockingError& d)
{
    Q_ASSERT(d.response && d.choices != DBlockingError::Choice::NoChoice);
    auto btns = choicesToButtons(d.choices);
    auto def = smChoiceButtonMap.toRight(d.defaultChoice);
    int rawRes = Qx::postBlockingError(d.error, btns, def);
    *d.response = smChoiceButtonMap.toLeft(static_cast<QMessageBox::StandardButton>(rawRes));
}

void FrontendGui::handleSaveFilename(const DSaveFilename& d)
{
    Q_ASSERT(d.response);
    *d.response = QFileDialog::getSaveFileName(nullptr, d.caption, d.dir, d.filter, d.selectedFilter);
}

void FrontendGui::handleExistingDir(const DExistingDir& d)
{
    Q_ASSERT(d.response);
    *d.response = QFileDialog::getExistingDirectory(nullptr, d.caption, d.startingDir);
}

void FrontendGui::handleItemSelection(const DItemSelection& d)
{
    Q_ASSERT(d.response);
    *d.response = QInputDialog::getItem(nullptr, d.caption, d.label, d.items, 0, false);
}

void FrontendGui::handleYesOrNo(const DYesOrNo& d)
{
    Q_ASSERT(d.response);
    *d.response = QMessageBox::question(nullptr, QString(), d.question) == QMessageBox::Yes;
}

bool FrontendGui::aboutToExit()
{
    // Quit once no windows remain
    if(windowsAreOpen())
    {
        connect(qApp, &QGuiApplication::lastWindowClosed, this, &FrontendGui::exit);
        return false;
    }
    else
        return true;
}

void FrontendGui::setupTrayIcon()
{
    // Set Icon
    mTrayIcon.setIcon(appIconFromResources());

    // Set ToolTip Action
    mTrayIcon.setToolTip(SYS_TRAY_STATUS);
    connect(&mTrayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
        if(reason != QSystemTrayIcon::Context)
            mTrayIcon.showMessage(mStatusHeading, mStatusMessage);
    });

    // Set Context Menu
    QAction* quit = new QAction(&mTrayIconContextMenu);
    quit->setIcon(trayExitIconFromResources());
    quit->setText(u"Quit"_s);
    connect(quit, &QAction::triggered, this, [this]{
        // Notify driver to quit if it still exists
        shutdownDriver();

        // Close all top-level windows
        qApp->closeAllWindows();
    });
    mTrayIconContextMenu.addAction(quit);
    mTrayIcon.setContextMenu(&mTrayIconContextMenu);

    // Display Icon
    mTrayIcon.show();
}

void FrontendGui::setupProgressDialog()
{
    // Initialize dialog
    mProgressDialog.setCancelButtonText(u"Cancel"_s);
    mProgressDialog.setWindowModality(Qt::NonModal);
    mProgressDialog.setMinimumDuration(0);
    mProgressDialog.setAutoClose(true);
    mProgressDialog.setAutoReset(false);
    mProgressDialog.reset(); // Stops the auto-show timer that is started by QProgressDialog's ctor
    connect(&mProgressDialog, &QProgressDialog::canceled, this, [this]{
        /* A bit of a bodge. Pressing the Cancel button on a progress dialog
         * doesn't count as closing it (it doesn't fire a close event) by the strict definition of
         * QWidget, so here when the progress bar is closed we manually check to see if it was
         * the last window if the application is ready to exit.
         *
         * Normally the progress bar should never still be open by that point, but this is here as
         * a fail-safe as otherwise the application would deadlock when the progress bar is closed
         * via the Cancel button.
         */
        if(readyToExit() && !windowsAreOpen())
            exit();
        else
            cancelDriverTask();
    });
}
