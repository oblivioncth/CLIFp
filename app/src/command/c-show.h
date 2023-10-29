#ifndef CSHOW_H
#define CSHOW_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/command.h"

class QX_ERROR_TYPE(CShowError, "CShowError", 1217)
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
        {NoError, u""_s},
        {MissingThing, u"No message or extra to show was provided."_s}
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
    static inline const QString STATUS_SHOW_MSG = u"Displaying message"_s;
    static inline const QString STATUS_SHOW_EXTRA = u"Displaying extra"_s;

    // Command line option strings
    static inline const QString CL_OPT_MSG_S_NAME = u"m"_s;
    static inline const QString CL_OPT_MSG_L_NAME = u"message"_s;
    static inline const QString CL_OPT_MSG_DESC = u"Displays an pop-up dialog with the supplied message. Used primarily for some additional apps."_s;

    static inline const QString CL_OPT_EXTRA_S_NAME = u"e"_s;
    static inline const QString CL_OPT_EXTRA_L_NAME = u"extra"_s;
    static inline const QString CL_OPT_EXTRA_DESC = u"Opens an explorer window to the specified extra. Used primarily for some additional apps."_s;

    // Command line options
    static inline const QCommandLineOption CL_OPTION_MSG{{CL_OPT_MSG_S_NAME, CL_OPT_MSG_L_NAME}, CL_OPT_MSG_DESC, u"message"_s}; // Takes value
    static inline const QCommandLineOption CL_OPTION_EXTRA{{CL_OPT_EXTRA_S_NAME, CL_OPT_EXTRA_L_NAME}, CL_OPT_EXTRA_DESC, u"extra"_s}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_MSG, &CL_OPTION_EXTRA};

public:
    // Meta
    static inline const QString NAME = u"show"_s;
    static inline const QString DESCRIPTION = u"Display a message or extra folder"_s;

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
