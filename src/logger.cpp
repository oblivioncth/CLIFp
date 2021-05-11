#include "logger.h"

//===============================================================================================================
// LOGGER
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
Logger::Logger(QString filePath, QString rawCL, QString interpCL, QString header, int maxEntries)
    : mFilePath(filePath), mRawCommandLine(rawCL), mInterpretedCommandLine(interpCL),
      mEntryHeader(HEADER_TEMPLATE.arg(header)), mMaxEntries(maxEntries), mTimeStamp(QDateTime::currentDateTime())
{}

//-Instance Functions--------------------------------------------------------------------------------------------
//Public:
void Logger::appendErrorEvent(Qx::GenericError error)
{
    QString errorString = "[" + ERROR_LEVEL_STR_MAP.value(error.errorLevel()) + "] " + error.primaryInfo();
    if(!error.secondaryInfo().isNull())
        errorString + " " + error.secondaryInfo();
    if(!error.detailedInfo().isNull())
        errorString + " " + error.detailedInfo();

    mEvents.append(EVENT_TEMPLATE.arg(QTime::currentTime().toString(), errorString));
}

void Logger::appendGeneralEvent(QString event) { mEvents.append(EVENT_TEMPLATE.arg(QTime::currentTime().toString(), event)); }

Qx::IOOpReport Logger::finish(int returnCode)
{
    //-Construct Entry---------------------------------------------------
    QString entry;

    // Header
    entry += ENTRY_START_TEMPLATE.arg(mEntryHeader, mTimeStamp.toString()) + "\n";

    // Start parameters
    entry += RAW_CL_LABEL + " " + mRawCommandLine + "\n";
    entry += INTERP_CL_LABEL + " " + mInterpretedCommandLine + "\n";

    // Exit code
    entry += EXIT_CODE_LABEL + " " + QString::number(returnCode) + "\n";

    // Entries
    entry += EVENTS_LABEL;
    for(const QString& event : mEvents)
        entry += "\n - " + event;

    //-Prepare Log File--------------------------------------------------
    QFile logFile(mFilePath);
    QFileInfo logFileInfo(logFile);
    Qx::IOOpReport logFileOpReport;

    //-Handle Formating For Existing Log---------------------------------
    if(logFileInfo.exists() && logFileInfo.isFile() && !Qx::fileIsEmpty(logFile))
    {
        // Add spacer to start of new entry
        entry.prepend('\n');

        // Get entry count and locations
        QList<Qx::TextPos> entryStartLocations;
        logFileOpReport = Qx::findStringInFile(entryStartLocations, logFile, mEntryHeader);
        if(!logFileOpReport.wasSuccessful())
            return logFileOpReport;

        // Purge oldest entries if current count is at or above limit
        if(entryStartLocations.count() >= mMaxEntries)
        {
            int firstToKeep = entryStartLocations.count() - mMaxEntries + 1; // +1 to account for new entry
            Qx::TextPos deleteEnd = Qx::TextPos(entryStartLocations.at(firstToKeep).getLineNum() - 1, -1);
            logFileOpReport = Qx::deleteTextRangeFromFile(logFile, Qx::TextPos::START, deleteEnd);
            if(!logFileOpReport.wasSuccessful())
                return logFileOpReport;
        }
    }

    //-Append Entry to Log----------------------------------------------
    logFileOpReport = Qx::writeStringToEndOfFile(logFile, entry, true, true);
    if(!logFileOpReport.wasSuccessful())
        return logFileOpReport;

    // Return Success
    return Qx::IOOpReport(Qx::IO_OP_WRITE, Qx::IO_SUCCESS, logFile);
}
