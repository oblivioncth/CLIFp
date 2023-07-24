// Unit Include
#include "utility.h"

// Qt Includes
#include <QDirIterator>
#include <QStandardPaths>

// Project Includes
#include "project_vars.h"

//-Macros----------------------------------------
#define APP_ICON_RES_BASE_PATH u":/app/"_s

using namespace Qt::Literals::StringLiterals;

namespace Utility
{

namespace
{
    const QString dimStr(int w, int h)
    {
        static const QString dimTemplate = u"%1x%2"_s;
        return dimTemplate.arg(w).arg(h);
    }

    const QString appIconResourceFullPath(int w, int h)
    {
        return APP_ICON_RES_BASE_PATH + dimStr(w, h) + '/' + PROJECT_SHORT_NAME u".png"_s;
    }

    const QList<QSize>& availableAppIconSizes()
    {
        static QList<QSize> sizes;
        if(sizes.isEmpty())
        {
            QDirIterator itr(APP_ICON_RES_BASE_PATH, QDir::Dirs | QDir::NoDotAndDotDot);
            while(itr.hasNext())
            {
                itr.next();
                QStringList dims = itr.fileName().split('x');
                int w = dims.first().toInt();
                int h = dims.last().toInt();
                sizes << QSize(w, h);
            }
        }
        return sizes;
    }
}

//-Functions----------------------------------------------------
const QIcon& appIconFromResources()
{
    static QIcon appIcon;
    if(appIcon.isNull())
    {
        const QList<QSize>& sizes = availableAppIconSizes();
        for(const QSize& size : sizes)
            appIcon.addFile(appIconResourceFullPath(size.width(), size.height()), size);
    }
    return appIcon;
}

bool installAppIconForUser()
{
    static const QDir iconDestBaseDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/icons/hicolor"_s);

    const QList<QSize>& sizes = availableAppIconSizes();
    for(const QSize& size : sizes)
    {
        QString resSpecificSubPath = dimStr(size.width(), size.height()) + u"/apps"_s;

        // Ensure path exists
        iconDestBaseDir.mkpath(u"./"_s + resSpecificSubPath);

        // Determine paths
        QString fullSrcPath = appIconResourceFullPath(size.width(), size.height());
        QString fullDestPath = iconDestBaseDir.absolutePath() + '/' + resSpecificSubPath + '/' + PROJECT_SHORT_NAME + u".png"_s;

        // Remove exiting file if it exists (icon could need to be updated), then copy the new icon
        if((QFile::exists(fullDestPath) && !QFile::remove(fullDestPath) ) || !QFile::copy(fullSrcPath, fullDestPath))
            return false;
    }

    return true;
}

}
