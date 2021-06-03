#include "logger.h"

//===============================================================================================================
// LOGGER
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
Logger::Logger(QFile* const logFile, QString rawCL, QString interpCL, QString header, int maxEntries)
    : mLogFile(logFile), mRawCommandLine(rawCL), mInterpretedCommandLine(interpCL),
      mEntryHeader(HEADER_TEMPLATE.arg(header)), mTimeStamp(QDateTime::currentDateTime()), mMaxEntries(maxEntries)
{
    // Initializer stream writer
    mTextStreamWriter = std::make_unique<Qx::TextStreamWriter>(*mLogFile);
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Public:
Qx::IOOpReport Logger::openLog()
{
    //-Prepare Log File--------------------------------------------------
    QFileInfo logFileInfo(*mLogFile);
    Qx::IOOpReport logFileOpReport;

    QString entryStart;

    //-Handle Formating For Existing Log---------------------------------
    if(logFileInfo.exists() && logFileInfo.isFile() && !Qx::fileIsEmpty(*mLogFile))
    {
        // Add spacer to start of new entry
        entryStart.prepend('\n');

        // Get entry count and locations
        QList<Qx::TextPos> entryStartLocations;
        logFileOpReport = Qx::findStringInFile(entryStartLocations, *mLogFile, mEntryHeader);
        if(!logFileOpReport.wasSuccessful())
        {
            mErrorStatus = logFileOpReport;
            return logFileOpReport;
        }

        // Purge oldest entries if current count is at or above limit
        if(entryStartLocations.count() >= mMaxEntries)
        {
            int firstToKeep = entryStartLocations.count() - mMaxEntries + 1; // +1 to account for new entry
            Qx::TextPos deleteEnd = Qx::TextPos(entryStartLocations.at(firstToKeep).getLineNum() - 1, -1);
            logFileOpReport = Qx::deleteTextRangeFromFile(*mLogFile, Qx::TextPos::START, deleteEnd);
            if(!logFileOpReport.wasSuccessful())
            {
                mErrorStatus = logFileOpReport;
                return logFileOpReport;
            }
        }
    }

    // Open log through stream
    logFileOpReport = mTextStreamWriter->openFile();
    if(!logFileOpReport.wasSuccessful())
    {
        mErrorStatus = logFileOpReport;
        return logFileOpReport;
    }

    // Construct entry start
    // Header
    entryStart += ENTRY_START_TEMPLATE.arg(mEntryHeader, mTimeStamp.toString()) + "\n";

    // Start parameters
    entryStart += RAW_CL_LABEL + " " + mRawCommandLine + "\n";
    entryStart += INTERP_CL_LABEL + " " + mInterpretedCommandLine + "\n";

    // Write start of entry
    logFileOpReport = mTextStreamWriter->writeText(entryStart);
    return logFileOpReport;
}

Qx::IOOpReport Logger::recordErrorEvent(Qx::GenericError error)
{
    if(mErrorStatus.wasSuccessful())
    {
        QString errorString = EVENT_TEMPLATE.arg(QTime::currentTime().toString(), "[" + ERROR_LEVEL_STR_MAP.value(error.errorLevel()) + "] " + error.primaryInfo());
        if(!error.secondaryInfo().isNull())
            errorString + " " + error.secondaryInfo();
        if(!error.detailedInfo().isNull())
            errorString + "\n\t" + error.detailedInfo().replace("\n", "\n\t");

        mErrorStatus = mTextStreamWriter->writeLine(errorString);
        return mErrorStatus;
    }

    return Qx::IOOpReport();
}

Qx::IOOpReport Logger::recordGeneralEvent(QString event)
{
    if(mErrorStatus.wasSuccessful())
    {
        mErrorStatus = mTextStreamWriter->writeLine(EVENT_TEMPLATE.arg(QTime::currentTime().toString(), event));
        return mErrorStatus;
    }

    return Qx::IOOpReport();
}

Qx::IOOpReport Logger::finish(int returnCode)
{
    if(mErrorStatus.wasSuccessful())
    {
        // Print exit code
        mErrorStatus = mTextStreamWriter->writeLine(FINISH_TEMPLATE.arg(returnCode == 0 ? FINISH_SUCCESS : FINISH_ERR).arg(returnCode));

        // Close log
        mTextStreamWriter->closeFile();
        return mErrorStatus;
    }

    return Qx::IOOpReport();
}

Qx::IOOpReport Logger::error() { return mErrorStatus; }
bool Logger::hasError() { return !mErrorStatus.wasSuccessful(); }
