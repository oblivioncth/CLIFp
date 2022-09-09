// Unit Include
#include "t-extra.h"

// Qt Includes
#include <QDesktopServices>
#include <QUrl>

//===============================================================================================================
// TExtra
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TExtra::TExtra() {}

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
    ErrorCode errorStatus = ErrorCode::NO_ERR;

    // Ensure extra exists
    if(mDirectory.exists())
    {
        // Open extra
        QDesktopServices::openUrl(QUrl::fromLocalFile(mDirectory.absolutePath()));
        emit eventOccurred(NAME, LOG_EVENT_SHOW_EXTRA.arg(QDir::toNativeSeparators(mDirectory.path())));
    }
    else
    {
        emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_EXTRA_NOT_FOUND.arg(QDir::toNativeSeparators(mDirectory.path()))));
        errorStatus = ErrorCode::EXTRA_NOT_FOUND;
    }

    emit complete(errorStatus);
}
