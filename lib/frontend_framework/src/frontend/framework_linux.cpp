// Unit Include
#include "frontend/framework.h"

// Qt Includes
#include <QDir>
#include <QStandardPaths>
#include <QIcon>
#include <QImageReader>
#include <QImageWriter>

// Project Includes
#include "_frontend_project_vars.h"

namespace
{

const QString dimStr(int w, int h)
{
    static const QString dimTemplate = u"%1x%2"_s;
    return dimTemplate.arg(w).arg(h);
}

}

//===============================================================================================================
// Framework
//===============================================================================================================

//-Class Functions-----------------------------------------------------------------------------
//Protected:
bool FrontendFramework::updateUserIcons()
{
    static const QString HASH_KEY = u"CLIFP_ICO_HASH"_s;
    static const QDir ICON_DEST_BASE_DIR(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/icons/hicolor"_s);

    QImageReader imgReader;
    QImageWriter imgWriter;

    const QIcon& appIcon = appIconFromResources();
    for(const QSize& size : appIcon.availableSizes())
    {
        QString resSpecificSubPath = dimStr(size.width(), size.height()) + u"/apps"_s;

        // Ensure path exists
        ICON_DEST_BASE_DIR.mkpath(u"./"_s + resSpecificSubPath);

        // Determine dest
        QFile destFile(ICON_DEST_BASE_DIR.absolutePath() + '/' + resSpecificSubPath + '/' + PROJECT_SHORT_NAME + u".png"_s);

        if(destFile.exists())
        {
            // Check if file seems up-to-date
            imgReader.setDevice(&destFile);
            if(imgReader.text(HASH_KEY) == PROJECT_APP_ICO_HASH)
                continue;

            // Remove old file
            if(!destFile.remove())
                return false;
        }

        // Synthesize specifc size image
        QImage img = appIcon.pixmap(size).toImage();

        // Write image
        imgWriter.setDevice(&destFile);
        imgWriter.setText(HASH_KEY, PROJECT_APP_ICO_HASH); // I think we have to do this each time after changing the file
        if(!imgWriter.write(img))
            return false;
    }

    return true;
}
