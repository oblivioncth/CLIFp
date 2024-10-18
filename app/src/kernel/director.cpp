// Unit Include
#include "director.h"

// Qt Includes
#include <QCoreApplication>

// Project Includes
#include "project_vars.h"
#include "utility.h"
#include "task/task.h"

//===============================================================================================================
// DirectorError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
DirectorError::DirectorError(Type t, const QString& s, Qx::Severity sv) :
    mType(t),
    mSpecific(s),
    mSeverity(sv)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool DirectorError::isValid() const { return mType != NoError; }
QString DirectorError::specific() const { return mSpecific; }
DirectorError::Type DirectorError::type() const { return mType; }

//Private:
Qx::Severity DirectorError::deriveSeverity() const { return mSeverity; }
quint32 DirectorError::deriveValue() const { return mType; }
QString DirectorError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString DirectorError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// Director
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
Director::Director() :
    mLogger(CLIFP_DIR_PATH + '/' + CLIFP_CUR_APP_BASENAME  + '.' + LOG_FILE_EXT),
    mVerbosity(Verbosity::Full),
    mCriticalErrorOccurred(false)
{
    bool established = establishCanonDirector(*this);
    Q_ASSERT(established);  // No reason for more than one Director currently

    mLogger.setApplicationName(PROJECT_SHORT_NAME);
    mLogger.setApplicationVersion(PROJECT_VERSION_STR);
    mLogger.setMaximumEntries(LOG_MAX_ENTRIES);
}

//-Class Functions------------------------------------------------------------------------------------------------------
//Private:
bool Director::establishCanonDirector(Director& cd)
{
    if(!smDefaultMessageHandler)
        smDefaultMessageHandler = qInstallMessageHandler(qtMessageHandler);

    if(smCanonDirector)
        return false;

    smCanonDirector = &cd;
    return true;
}

void Director::qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    // Log messages
    if(smCanonDirector && smCanonDirector->isLogOpen())
        smCanonDirector->logQtMessage(type, context, msg);

    // Defer to default behavior
    if(smDefaultMessageHandler)
        smDefaultMessageHandler(type, context, msg);
}

//-Instance Functions-------------------------------------------------------------
//Private:
template<typename F>
    requires Qx::defines_call_for_s<F, Qx::IoOpReport>
void Director::log(F logFunc)
{
    if(mLogger.hasError())
        return;

    if(auto r = logFunc(); r.isFailure())
        postDirective(NAME, DError{Qx::Error(r).setSeverity(Qx::Warning)});
}


bool Director::isLogOpen() const { return mLogger.isOpen(); }

void Director::logQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
#if defined QT_NO_MESSAGELOGCONTEXT || !defined QT_MESSAGELOGCONTEXT
    QString msgWithContext = msg;
#else
    static const QString cTemplate = u"(%1:%2, %3) %4"_s;
    static const QString unk = u"Unk."_s;
    QString msgWithContext = cTemplate.arg(
        context.file ? QString(context.file) : unk,
        context.line >= 0 ? QString::number(context.line) : unk,
        context.function ? QString(context.function) : unk,
        msg
        );
#endif

    switch (type)
    {
    case QtDebugMsg:
        logEvent(NAME, u"SYSTEM DEBUG) "_s + msgWithContext);
        break;
    case QtInfoMsg:
        logEvent(NAME, u"SYSTEM INFO) "_s + msgWithContext);
        break;
    case QtWarningMsg:
        logError(NAME, DirectorError(DirectorError::InternalError, msgWithContext, Qx::Warning));
        break;
    case QtCriticalMsg:
        logError(NAME, DirectorError(DirectorError::InternalError, msgWithContext, Qx::Err));
        break;
    case QtFatalMsg:
        logError(NAME, DirectorError(DirectorError::InternalError, msgWithContext, Qx::Critical));
        break;
    }
}

//Public:
Director::Verbosity Director::verbosity() const { return mVerbosity; }
bool Director::criticalErrorOccurred() const { return mCriticalErrorOccurred; }

void Director::openLog(const QStringList& arguments)
{
    mLogger.setApplicationArguments(arguments);
    log([&](){ return mLogger.openLog(); });
}

void Director::setVerbosity(Verbosity verbosity)
{
    mVerbosity = verbosity;
    logEvent(NAME, LOG_EVENT_NOTIFCATION_LEVEL.arg(ENUM_NAME(verbosity)));
}

void Director::logCommand(const QString& src, const QString& commandName)
{
    log([&](){ return mLogger.recordGeneralEvent(src, COMMAND_LABEL.arg(commandName)); });
}

void Director::logCommandOptions(const QString& src, const QString& commandOptions)
{
    log([&](){ return mLogger.recordGeneralEvent(src, COMMAND_OPT_LABEL.arg(commandOptions)); });
}

void Director::logError(const QString& src, const Qx::Error& error)
{
    log([&](){ return mLogger.recordErrorEvent(src, error); });

    if(error.severity() == Qx::Critical)
        mCriticalErrorOccurred = true;
}

void Director::logEvent(const QString& src, const QString& event)
{
    log([&](){ return mLogger.recordGeneralEvent(src, event); });
}


// TODO: Have task have a toString function/operator instead of "members()"
void Director::logTask(const QString& src, const Task* task) { logEvent(src, LOG_EVENT_TASK_ENQ.arg(task->name(), task->members().join(u", "_s))); }

ErrorCode Director::logFinish(const QString& src, const Qx::Error& errorState)
{
    if(mCriticalErrorOccurred)
        logEvent(src, LOG_ERR_CRITICAL);

    ErrorCode code = errorState.typeCode();

    log([&](){ return mLogger.finish(code); });

    // Return exit code so main function can return with this one
    return code;
}


