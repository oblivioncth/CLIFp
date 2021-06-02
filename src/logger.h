#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QStringList>
#include "qx.h"
#include "qx-io.h"

class Logger
{
//-Class Variables----------------------------------------------------------------------------------------------------
private:
    static inline const QString HEADER_TEMPLATE = "[ %1 ]";
    static inline const QString ENTRY_START_TEMPLATE = "%1 : %2";
    static inline const QString EVENT_TEMPLATE = "<%1> %2";
    static inline const QString RAW_CL_LABEL = "Raw Parameters:";
    static inline const QString INTERP_CL_LABEL = "Interpreted Parameters:";
    static inline const QString EXIT_CODE_LABEL = "Exit Code:";
    static inline const QString EVENTS_LABEL = "Events:";

    static inline const QHash<Qx::GenericError::ErrorLevel, QString> ERROR_LEVEL_STR_MAP = {
        {Qx::GenericError::Undefined, "UNDEF_LEVEL"},
        {Qx::GenericError::Warning, "WARNING"},
        {Qx::GenericError::Error, "ERROR"},
        {Qx::GenericError::Critical, "CRITICAL"}
    };

//-Instance Variables-------------------------------------------------------------------------------------------------
private:
    // From constructor
    QFile const* mLogFile;
    QString mRawCommandLine;
    QString mInterpretedCommandLine;
    QString mEntryHeader;
    QDateTime mTimeStamp;
    int mMaxEntries;

    // Working var
    std::unique_ptr<Qx::TextStreamWriter> mTextStreamWriter;

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Logger(QFile* const logFile, QString rawCL, QString interpCL, QString header, int maxEntries);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    Qx::IOOpReport openLog();
    void recordErrorEvent(Qx::GenericError error);
    void recordGeneralEvent(QString event);
    Qx::IOOpReport closeLog(int returnCode);
};

#endif // LOGGER_H
