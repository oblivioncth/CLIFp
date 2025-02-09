#ifndef PROGRESSPRINTER_H
#define PROGRESSPRINTER_H

// Qt Includes
#include <QtTypes>
#include <QString>

// Qx Includes
#include <qx/core/qx-iostream.h>

class ProgressPrinter
{
//-Instance Variables----------------------------------------------------------------------------------------------
private:
    qint64 mCurrent;
    qint64 mMax;
    qint64 mScaled;

    QString mProcedure; // NOTE: Unused
    bool mActive;
    bool mHoldsEndlCtrl;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    ProgressPrinter();

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    void rescale();
    void print();
    void reset();

public:
    void start(const QString& procedure);
    void finish();

    void setValue(qint64 value);
    void setMaximum(qint64 max);
    bool isActive() const;
    bool checkAndResetEndlCtrl();
};

#endif // PROGRESSPRINTER_H
