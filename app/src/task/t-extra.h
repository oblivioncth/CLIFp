#ifndef TEXTRA_H
#define TEXTRA_H

// Qt Includes
#include <QDir>

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"

class QX_ERROR_TYPE(TExtraError, "TExtraError", 1254)
{
    friend class TExtra;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        NotFound = 1
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, QSL("")},
        {NotFound, QSL("Could not find an extra to display.")}
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    TExtraError(Type t = NoError, const QString& s = {});

    //-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

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
    TExtra(QObject* parent);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    const QDir directory() const;

    void setDirectory(QDir dir);

    void perform() override;
};

#endif // TEXTRA_H
