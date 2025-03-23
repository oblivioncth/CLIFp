#ifndef CPREPARE_H
#define CPREPARE_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/title-command.h"

class CPrepare : public TitleCommand
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_PREPARE = u"Preparing"_s;

    // Logging - Errors
    static inline const QString LOG_WRN_PREP_NOT_DATA_PACK = u"The provided ID does not belong to a Data Pack based game (%1). No action will be taken."_s;

public:
    // Meta
    static inline const QString NAME = u"prepare"_s;
    static inline const QString DESCRIPTION = u"Initializes Flashpoint for playing the provided Data Pack based title by UUID. If the title does not use a Data Pack this command has no effect."_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CPrepare(Core& coreRef, const QStringList& commandLine);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    QList<const QCommandLineOption*> options() const override;
    QString name() const override;

public:
    Qx::Error perform() override;

public:
    bool requiresServices() const override;
};

#endif // CPREPARE_H
