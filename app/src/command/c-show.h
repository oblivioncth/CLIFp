#ifndef CSHOW_H
#define CSHOW_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/command.h"

class QX_ERROR_TYPE(CShowError, "CShowError", 1216)
{
    friend class CShow;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        MissingThing = 1
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, QSL("")},
        {MissingThing, QSL("No message or extra to show was provided.")}
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    CShowError(Type t = NoError, const QString& s = {});

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

class CShow : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_SHOW_MSG = QSL("Displaying message");
    static inline const QString STATUS_SHOW_EXTRA = QSL("Displaying extra");

    // Command line option strings
    static inline const QString CL_OPT_MSG_S_NAME = QSL("m");
    static inline const QString CL_OPT_MSG_L_NAME = QSL("message");
    static inline const QString CL_OPT_MSG_DESC = QSL("Displays an pop-up dialog with the supplied message. Used primarily for some additional apps.");

    static inline const QString CL_OPT_EXTRA_S_NAME = QSL("e");
    static inline const QString CL_OPT_EXTRA_L_NAME = QSL("extra");
    static inline const QString CL_OPT_EXTRA_DESC = QSL("Opens an explorer window to the specified extra. Used primarily for some additional apps.");

    // Command line options
    static inline const QCommandLineOption CL_OPTION_MSG{{CL_OPT_MSG_S_NAME, CL_OPT_MSG_L_NAME}, CL_OPT_MSG_DESC, "message"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_EXTRA{{CL_OPT_EXTRA_S_NAME, CL_OPT_EXTRA_L_NAME}, CL_OPT_EXTRA_DESC, "extra"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_MSG, &CL_OPTION_EXTRA};

public:
    // Meta
    static inline const QString NAME = QSL("show");
    static inline const QString DESCRIPTION = QSL("Display a message or extra folder");

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CShow(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    QList<const QCommandLineOption*> options() override;
    QString name() override;
    Qx::Error perform() override;
};
REGISTER_COMMAND(CShow::NAME, CShow, CShow::DESCRIPTION);

#endif // CSHOW_H
