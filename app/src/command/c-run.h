#ifndef CRUN_H
#define CRUN_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/command.h"

class QX_ERROR_TYPE(CRunError, "CRunError", 1215)
{
    friend class CRun;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        MissingApp = 1
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, QSL("")},
        {MissingApp, QSL("No application to run was provided.")}
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    CRunError(Type t = NoError, const QString& s = {});

    //-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class CRun : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_RUN = QSL("Running");

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
    QList<const QCommandLineOption*> options() override;
    QString name() override;
    Qx::Error perform() override;
};
REGISTER_COMMAND(CRun::NAME, CRun, CRun::DESCRIPTION);

#endif // CRUN_H
