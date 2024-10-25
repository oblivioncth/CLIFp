// Unit Include
#include "statusrelay.h"

// Qt Includes
#include <QApplication>
#include <QScopedValueRollback>
#include <QFileDialog>
#include <QInputDialog>

// Qx Includes
#include <qx/widgets/qx-common-widgets.h>
#include <qx/utility/qx-helpers.h>

// Magic enum
#include "magic_enum_utility.hpp"

//===============================================================================================================
// STATUS RELAY
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
StatusRelay::StatusRelay(QObject* parent) :
    QObject(parent),
    mSystemClipboard(QGuiApplication::clipboard())
{
    setupTrayIcon();
    setupProgressDialog();
}

//-Class Functions----------------------------------------------------------------------------------------------------
//Private:
QMessageBox::StandardButtons StatusRelay::choicesToButtons(DBlockingError::Choices cs)
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
QMessageBox* StatusRelay::prepareMessageBox(const MessageT& dMsg)
{
    QMessageBox* msg  = new QMessageBox();
    msg->setIcon(QMessageBox::Information);
    msg->setWindowTitle(QApplication::applicationName()); // This should be the default, but hey being explicit never hurt
    msg->setText(dMsg.text);

    if(dMsg.selectable)
        msg->setTextInteractionFlags(Qt::TextSelectableByMouse);

    return msg;
}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
void StatusRelay::setupTrayIcon()
{
    // Set Icon
    mTrayIcon.setIcon(QIcon(u":/app/CLIFp.ico"_s));

    // Set ToolTip Action
    mTrayIcon.setToolTip(SYS_TRAY_STATUS);
    connect(&mTrayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
        if(reason != QSystemTrayIcon::Context)
            mTrayIcon.showMessage(mStatusHeading, mStatusMessage);
    });

    // Set Context Menu
    QAction* quit = new QAction(&mTrayIconContextMenu);
    quit->setIcon(QIcon(u":/tray/Exit.png"_s));
    quit->setText(u"Quit"_s);
    connect(quit, &QAction::triggered, this, &StatusRelay::quitRequested);
    mTrayIconContextMenu.addAction(quit);
    mTrayIcon.setContextMenu(&mTrayIconContextMenu);

    // Display Icon
    mTrayIcon.show();
}

void StatusRelay::setupProgressDialog()
{
    // Initialize dialog
    mLongTaskProgressDialog.setCancelButtonText(u"Cancel"_s);
    mLongTaskProgressDialog.setWindowModality(Qt::NonModal);
    mLongTaskProgressDialog.setMinimumDuration(0);
    mLongTaskProgressDialog.setAutoClose(true);
    mLongTaskProgressDialog.setAutoReset(false);
    mLongTaskProgressDialog.reset(); // Stops the auto-show timer that is started by QProgressDialog's ctor
    connect(&mLongTaskProgressDialog, &QProgressDialog::canceled, this, &StatusRelay::longTaskCanceled);
}

void StatusRelay::handleMessage(const DMessage& d)
{
    auto mb = prepareMessageBox(d);
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
}

void StatusRelay::handleError(const DError& d) { Qx::postError(d.error); }

void StatusRelay::handleProcedureStart(const DProcedureStart& d)
{
    // Set label
    mLongTaskProgressDialog.setLabelText(d.label);

    // Show right away
    mLongTaskProgressDialog.setValue(0);
}

void StatusRelay::handleProcedureStop(const DProcedureStop& d)
{
    Q_UNUSED(d);
    /* Always reset the dialog regardless of whether it is visible or not as it may not be currently visible,
     * but queued to be visible on the next event loop iteration and therefore still needs to be hidden
     * immediately after.
     */
    mLongTaskProgressDialog.reset();
}

void StatusRelay::handleProcedureProgress(const DProcedureProgress& d) { mLongTaskProgressDialog.setValue(d.current); }
void StatusRelay::handleProcedureScale(const DProcedureScale& d) { mLongTaskProgressDialog.setMaximum(d.max); }
void StatusRelay::handleClipboardUpdate(const DClipboardUpdate& d) { mSystemClipboard->setText(d.text); }
void StatusRelay::handleStatusUpdate(const DStatusUpdate& d) { mStatusHeading = d.heading; mStatusMessage = d.message; }

// Sync directive handlers
void StatusRelay::handleBlockingMessage(const DBlockingMessage& d)
{
    auto mb = prepareMessageBox(d);
    mb->exec();
    delete mb;
}

void StatusRelay::handleBlockingError(const DBlockingError& d)
{
    Q_ASSERT(d.response);
    auto btns = choicesToButtons(d.choices);
    auto def = smChoiceButtonMap.toRight(d.defaultChoice);
    int rawRes = Qx::postBlockingError(d.error, btns, def);
    *d.response = smChoiceButtonMap.toLeft(static_cast<QMessageBox::StandardButton>(rawRes));
}

void StatusRelay::handleSaveFilename(const DSaveFilename& d)
{
    Q_ASSERT(d.response);
    *d.response = QFileDialog::getSaveFileName(nullptr, d.caption, d.dir, d.filter, d.selectedFilter);
}

void StatusRelay::handleExistingDir(const DExistingDir& d)
{
    Q_ASSERT(d.response);
    *d.response = QFileDialog::getExistingDirectory(nullptr, d.caption, d.startingDir);
}

void StatusRelay::handleItemSelection(const DItemSelection& d)
{
    Q_ASSERT(d.response);
    *d.response = QInputDialog::getItem(nullptr, d.caption, d.label, d.items, 0, false);
}

void StatusRelay::handleYesOrNo(const DYesOrNo& d)
{
    Q_ASSERT(d.response);
    *d.response = QMessageBox::question(nullptr, QString(), d.question) == QMessageBox::Yes;
}

//-Signals & Slots-------------------------------------------------------------
//Public Slots:
void StatusRelay::asyncDirectiveHandler(const AsyncDirective& aDirective)
{
    std::visit(qxFuncAggregate{
        [this](DMessage d){ handleMessage(d); },
        [this](DError d) { handleError(d); },
        [this](DProcedureStart d) { handleProcedureStart(d); },
        [this](DProcedureStop d) { handleProcedureStop(d); },
        [this](DProcedureProgress d) { handleProcedureProgress(d); },
        [this](DProcedureScale d) { handleProcedureScale(d); },
        [this](DClipboardUpdate d) { handleClipboardUpdate(d); },
        [this](DStatusUpdate d) { handleStatusUpdate(d); },
    }, aDirective);
}

void StatusRelay::syncDirectiveHandler(const SyncDirective& sDirective)
{
    std::visit(qxFuncAggregate{
        [this](DBlockingMessage d){ handleBlockingMessage(d); },
        [this](DBlockingError d){ handleBlockingError(d); },
        [this](DSaveFilename d) { handleSaveFilename(d); },
        [this](DExistingDir d) { handleExistingDir(d); },
        [this](DItemSelection d) { handleItemSelection(d); },
        [this](DYesOrNo d) { handleYesOrNo(d); }
    }, sDirective);
}
