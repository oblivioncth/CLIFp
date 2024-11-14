#ifndef GUI_H
#define GUI_H

// Qt Includes
#include <QMessageBox>
#include <QProgressDialog>
#include <QSystemTrayIcon>
#include <QMenu>

// Qx Includes
#include <qx/core/qx-bimap.h>
#include <qx/utility/qx-concepts.h>

// Project Includes
#include "frontend/framework.h"

class FrontendGui final : public FrontendFramework
{
//-Class Variables--------------------------------------------------------------------------------------------------------
private:
    static inline const Qx::Bimap<DBlockingError::Choice, QMessageBox::StandardButton> smChoiceButtonMap{
        {DBlockingError::Choice::Ok, QMessageBox::Ok},
        {DBlockingError::Choice::Yes, QMessageBox::Yes},
        {DBlockingError::Choice::No, QMessageBox::No},
    };

    static inline const QString SYS_TRAY_STATUS = u"CLIFp is running"_s;

//-Instance Variables----------------------------------------------------------------------------------------------
private:
    QProgressDialog mProgressDialog;
    QString mStatusHeading;
    QString mStatusMessage;

    QSystemTrayIcon mTrayIcon;
    QMenu mTrayIconContextMenu;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    explicit FrontendGui(QApplication* app);

//-Class Functions--------------------------------------------------------------------------------------------------------
//Private:
static QMessageBox::StandardButtons choicesToButtons(DBlockingError::Choices cs);

template<class MessageT>
    requires Qx::any_of<MessageT, DMessage, DBlockingMessage>
static QMessageBox* prepareMessageBox(const MessageT& dMsg);

static bool windowsAreOpen();

static QIcon& trayExitIconFromResources();

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Async directive handlers
    void handleDirective(const DMessage& d) override;
    void handleDirective(const DError& d) override;
    void handleDirective(const DProcedureStart& d) override;
    void handleDirective(const DProcedureStop& d) override;
    void handleDirective(const DProcedureProgress& d) override;
    void handleDirective(const DProcedureScale& d) override;
    void handleDirective(const DStatusUpdate& d) override;

    // Sync directive handlers
    void handleDirective(const DBlockingMessage& d) override;

    // Request directive handlers
    void handleDirective(const DBlockingError& d, DBlockingError::Choice* response) override;
    void handleDirective(const DSaveFilename& d, QString* response) override;
    void handleDirective(const DExistingDir& d, QString* response) override;
    void handleDirective(const DItemSelection& d, QString* response) override;
    void handleDirective(const DYesOrNo& d, bool* response) override;

    // Control
    bool aboutToExit() override;

    // Derived
    void setupTrayIcon();
    void setupProgressDialog();
};

#endif // GUI_H
