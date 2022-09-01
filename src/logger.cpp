// Unit Includes
#include "logger.h"

// Project Includes
#include "project_vars.h"

//===============================================================================================================
// LOGGER
//===============================================================================================================

//-Constructor---------------------------------------------------------------------------------------------------
//Public:
Logger::Logger(const QString& filePath, QString commandLine, QString globalOptions, QString header, int maxEntries)
    : mFilePath(filePath), mCommandLine(commandLine), mGlobalOptions(globalOptions),
      mEntryHeader(HEADER_TEMPLATE.arg(header, PROJECT_VERSION_STR)), mTimeStamp(QDateTime::currentDateTime()), mMaxEntries(maxEntries)
{
    // Initializer stream writer
    mTextStreamWriter = std::make_unique<Qx::TextStreamWriter>(mFilePath, Qx::WriteMode::Append, Qx::WriteOption::CreatePath |
                                                                                                Qx::WriteOption::Unbuffered);
}

//-Instance Functions--------------------------------------------------------------------------------------------
//Public:
Qx::IoOpReport Logger::openLog()
{
    //-Prepare Log File--------------------------------------------------
    QFile logFile(mFilePath);
    QFileInfo logFileInfo(logFile);
    Qx::IoOpReport logFileOpReport;

    QString entryStart;

    //-Handle Formatting For Existing Log---------------------------------
    if(logFileInfo.exists() && logFileInfo.isFile() && !Qx::fileIsEmpty(logFile))
    {
        // Add spacer to start of new entry
        entryStart.prepend('\n');

        // Get entry count and locations
        QList<Qx::TextPos> entryStartLocations;
        logFileOpReport = Qx::findStringInFile(entryStartLocations, logFile, mEntryHeader);
        if(logFileOpReport.isFailure())
        {
            mErrorStatus = logFileOpReport;
            return logFileOpReport;
        }

        // Purge oldest entries if current count is at or above limit
        if(entryStartLocations.count() >= mMaxEntries)
        {
            int firstToKeep = entryStartLocations.count() - mMaxEntries + 1; // +1 to account for new entry
            Qx::TextPos deleteEnd = Qx::TextPos(entryStartLocations.at(firstToKeep).line() - 1, Qx::Index32::LAST);
            logFileOpReport = Qx::deleteTextFromFile(logFile, Qx::TextPos::START, deleteEnd);
            if(logFileOpReport.isFailure())
            {
                mErrorStatus = logFileOpReport;
                return logFileOpReport;
            }
        }
    }

    // Open log through stream
    logFileOpReport = mTextStreamWriter->openFile();
    if(logFileOpReport.isFailure())
    {
        mErrorStatus = logFileOpReport;
        return logFileOpReport;
    }

    // Construct entry start
    // Header
    entryStart += ENTRY_START_TEMPLATE.arg(mEntryHeader, mTimeStamp.toString()) + "\n";

    // Start parameters
    entryStart += COMMANDLINE_LABEL + " " + mCommandLine + "\n";
    entryStart += GLOBAL_OPT_LABEL + " " + mGlobalOptions + "\n";

    // Write start of entry
    logFileOpReport = mTextStreamWriter->writeText(entryStart);
    return logFileOpReport;
}

Qx::IoOpReport Logger::recordVerbatim(QString text)
{
    if(!mErrorStatus.isFailure())
    {
        mErrorStatus = mTextStreamWriter->writeLine(text);
        return mErrorStatus;
    }

    return Qx::IoOpReport();
}

Qx::IoOpReport Logger::recordErrorEvent(QString src, Qx::GenericError error)
{
    if(!mErrorStatus.isFailure())
    {
        QString errorString = EVENT_TEMPLATE.arg(QTime::currentTime().toString(), src, ERROR_LEVEL_STR_MAP.value(error.errorLevel()) + ") " + error.primaryInfo());
        if(!error.secondaryInfo().isNull())
            errorString += " " + error.secondaryInfo();
        if(!error.detailedInfo().isNull())
            errorString += "\n\t" + error.detailedInfo().replace("\n", "\n\t");

        mErrorStatus = mTextStreamWriter->writeLine(errorString);
        return mErrorStatus;
    }

    return Qx::IoOpReport();
}

Qx::IoOpReport Logger::recordGeneralEvent(QString src, QString event)
{
    if(!mErrorStatus.isFailure())
    {
        mErrorStatus = mTextStreamWriter->writeLine(EVENT_TEMPLATE.arg(QTime::currentTime().toString(), src, event));
        return mErrorStatus;
    }

    return Qx::IoOpReport();
}

Qx::IoOpReport Logger::finish(int returnCode)
{
    if(!mErrorStatus.isFailure())
    {
        // Print exit code
        mErrorStatus = mTextStreamWriter->writeLine(FINISH_TEMPLATE.arg(returnCode == 0 ? FINISH_SUCCESS : FINISH_ERR).arg(returnCode));

        // Close log
        mTextStreamWriter->closeFile();
        return mErrorStatus;
    }

    return Qx::IoOpReport();
}

Qx::IoOpReport Logger::error() { return mErrorStatus; }
bool Logger::hasError() { return mErrorStatus.isFailure(); }
