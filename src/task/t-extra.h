#ifndef TEXTRA_H
#define TEXTRA_H

// Project Includes
#include "../task.h"

// Qt Includes
#include <QDir>

class TExtra : public Task
{
    Q_OBJECT;

//-Inner Classes--------------------------------------------------------------------------------------------------------
private:
    class ErrorCodes
    {
    //-Class Variables--------------------------------------------------------------------------------------------------
    public:
        static const ErrorCode NO_ERR = 0;
        static const ErrorCode EXTRA_NOT_FOUND = 20;
    };

//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TExtra");

    // Logging
    static inline const QString LOG_EVENT_SHOW_EXTRA = "Opened folder of extra %1";

    // Errors
    static inline const QString ERR_EXTRA_NOT_FOUND = "The extra %1 does not exist!";

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QDir mDirectory;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExtra();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    const QDir directory() const;

    void setDirectory(QDir dir);

    void perform() override;
};

#endif // TEXTRA_H
