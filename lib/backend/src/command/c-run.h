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
        NoError = 0
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s}
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
    static inline const QString STATUS_RUN = u"Running"_s;

    // Command line option strings
    static inline const QString CL_OPT_APP_S_NAME = u"a"_s;
    static inline const QString CL_OPT_APP_L_NAME = u"app"_s;
    static inline const QString CL_OPT_APP_DESC = u"Relative (to Flashpoint directory) path of application to launch."_s;

    static inline const QString CL_OPT_PARAM_S_NAME = u"p"_s;
    static inline const QString CL_OPT_PARAM_L_NAME = u"param"_s;
    static inline const QString CL_OPT_PARAM_DESC = u"Command-line parameters to use when starting the application."_s;

    // Command line options
    static inline const QCommandLineOption CL_OPTION_APP{{CL_OPT_APP_S_NAME, CL_OPT_APP_L_NAME}, CL_OPT_APP_DESC, u"application"_s}; // Takes value
    static inline const QCommandLineOption CL_OPTION_PARAM{{CL_OPT_PARAM_S_NAME, CL_OPT_PARAM_L_NAME}, CL_OPT_PARAM_DESC, u"parameters"_s}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_APP, &CL_OPTION_PARAM};
    static inline const QSet<const QCommandLineOption*> CL_OPTIONS_REQUIRED{&CL_OPTION_APP};

public:
    // Meta
    static inline const QString NAME = u"run"_s;
    static inline const QString DESCRIPTION = u"Start Flashpoint's webserver and then execute the provided application"_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CRun(Core& coreRef, const QStringList& commandLine);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    QList<const QCommandLineOption*> options() const override;
    QSet<const QCommandLineOption*> requiredOptions() const override;
    QString name() const override;

public:
    Qx::Error perform() override;

public:
    bool requiresServices() const override;
};

#endif // CRUN_H
