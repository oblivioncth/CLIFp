// Unit Include
#include "t-extra.h"

// Qt Includes
#include <QDesktopServices>
#include <QUrl>

//===============================================================================================================
// TExtraError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TExtraError::TExtraError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TExtraError::isValid() const { return mType != NoError; }
QString TExtraError::specific() const { return mSpecific; }
TExtraError::Type TExtraError::type() const { return mType; }

//Private:
Qx::Severity TExtraError::deriveSeverity() const { return Qx::Critical; }
quint32 TExtraError::deriveValue() const { return mType; }
QString TExtraError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TExtraError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// TExtra
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TExtra::TExtra(QObject* parent) :
    Task(parent)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TExtra::name() const { return NAME; }
QStringList TExtra::members() const
{
    QStringList ml = Task::members();
    ml.append(".directory() = \"" + QDir::toNativeSeparators(mDirectory.path()) + "\"");
    return ml;
}

const QDir TExtra::directory() const { return mDirectory; }

void TExtra::setDirectory(QDir dir) { mDirectory = dir; }

void TExtra::perform()
{
    // Error tracking
    TExtraError errorStatus;

    // Ensure extra exists
    if(mDirectory.exists())
    {
        // Open extra
        QDesktopServices::openUrl(QUrl::fromLocalFile(mDirectory.absolutePath()));
        emit eventOccurred(NAME, LOG_EVENT_SHOW_EXTRA.arg(QDir::toNativeSeparators(mDirectory.path())));
    }
    else
    {
        errorStatus = TExtraError(TExtraError::NotFound, QDir::toNativeSeparators(mDirectory.path()));
        emit errorOccurred(NAME, errorStatus);
    }

    emit complete(errorStatus);
}
