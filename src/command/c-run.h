#ifndef CRUN_H
#define CRUN_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/command.h"

class CRun : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_RUN = QSL("Running");

    // Error Messages - Prep
    static inline const QString ERR_NO_APP = QSL("No application to run was provided.");

    // Logging - Messages

    // Logging - Errors

    // Command line option strings
    static inline const QString CL_OPT_APP_S_NAME = QSL("a");
    static inline const QString CL_OPT_APP_L_NAME = QSL("app");
    static inline const QString CL_OPT_APP_DESC = QSL("Relative (to Flashpoint directory) path of application to launch.");

    static inline const QString CL_OPT_PARAM_S_NAME = QSL("p");
    static inline const QString CL_OPT_PARAM_L_NAME = QSL("param");
    static inline const QString CL_OPT_PARAM_DESC = QSL("Command-line parameters to use when starting the application.");

    // Command line options
    static inline const QCommandLineOption CL_OPTION_APP{{CL_OPT_APP_S_NAME, CL_OPT_APP_L_NAME}, CL_OPT_APP_DESC, "application"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_PARAM{{CL_OPT_PARAM_S_NAME, CL_OPT_PARAM_L_NAME}, CL_OPT_PARAM_DESC, "parameters"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_APP, &CL_OPTION_PARAM};

public:
    // Meta
    static inline const QString NAME = QSL("run");
    static inline const QString DESCRIPTION = QSL("Start Flashpoint's webserver and then execute the provided application");

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CRun(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    const QList<const QCommandLineOption*> options();
    const QString name();

public:
    ErrorCode process(const QStringList& commandLine);
};
REGISTER_COMMAND(CRun::NAME, CRun, CRun::DESCRIPTION);

#endif // CRUN_H
