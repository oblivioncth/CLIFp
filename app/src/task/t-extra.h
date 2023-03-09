#ifndef TEXTRA_H
#define TEXTRA_H

// Qt Includes
#include <QDir>

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"

class TExtra : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QSL("TExtra");

    // Logging
    static inline const QString LOG_EVENT_SHOW_EXTRA = QSL("Opened folder of extra %1");

    // Errors
    static inline const QString ERR_EXTRA_NOT_FOUND = QSL("The extra %1 does not exist!");

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QDir mDirectory;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExtra(QObject* parent = nullptr);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    const QDir directory() const;

    void setDirectory(QDir dir);

    void perform() override;
};

#endif // TEXTRA_H