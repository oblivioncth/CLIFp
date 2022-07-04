#ifndef LOGGER_H
#define LOGGER_H

// Qt Includes
#include <QString>
#include <QStringList>

// Qx Includes
#include <qx/core/qx-genericerror.h>
#include <qx/io/qx-textstreamwriter.h>

class Logger
{
//-Class Variables----------------------------------------------------------------------------------------------------
private:
    static inline const QString HEADER_TEMPLATE = "[ %1 ] (%2)";
    static inline const QString ENTRY_START_TEMPLATE = "%1 : %2";
    static inline const QString EVENT_TEMPLATE = " - <%1> [%2] %3";
    static inline const QString COMMANDLINE_LABEL = "Raw Parameters:";
    static inline const QString GLOBAL_OPT_LABEL = "Global Options:";
    static inline const QString EVENTS_LABEL = "Events:";
    static inline const QString FINISH_TEMPLATE = "---------- Execution finished %1 (Code %2) ----------";
    static inline const QString FINISH_SUCCESS = "successfully";
    static inline const QString FINISH_ERR = "prematurely";

    static inline const QHash<Qx::GenericError::ErrorLevel, QString> ERROR_LEVEL_STR_MAP = {
        {Qx::GenericError::Warning, "WARNING"},
        {Qx::GenericError::Error, "ERROR"},
        {Qx::GenericError::Critical, "CRITICAL"}
    };

//-Instance Variables-------------------------------------------------------------------------------------------------
private:
    // From constructor
    QFile* const mLogFile;
    QString mCommandLine;
    QString mGlobalOptions;
    QString mEntryHeader;
    QDateTime mTimeStamp;
    int mMaxEntries;

    // Working var
    std::unique_ptr<Qx::TextStreamWriter> mTextStreamWriter;
    Qx::IoOpReport mErrorStatus = Qx::IoOpReport();

//-Constructor--------------------------------------------------------------------------------------------------------
public:
    Logger(QFile* const logFile, QString commandLine, QString globalOptions, QString header, int maxEntries);

//-Instance Functions-------------------------------------------------------------------------------------------------
public:
    Qx::IoOpReport openLog();
    Qx::IoOpReport recordVerbatim(QString text);
    Qx::IoOpReport recordErrorEvent(QString src, Qx::GenericError error);
    Qx::IoOpReport recordGeneralEvent(QString src, QString event);
    Qx::IoOpReport finish(int returnCode);

    Qx::IoOpReport error();
    bool hasError();
};

#endif // LOGGER_H
