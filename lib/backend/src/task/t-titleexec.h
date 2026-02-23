#ifndef T_TITLEEXEC_H
#define T_TITLEEXEC_H

// Qt Includes

// Project Includes
#include "task/t-exec.h"

class TTitleExec : public TExec
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
// Meta
    static inline const QString NAME = u"TTitleExec"_s;

    // Logging


//-Instance Variables------------------------------------------------------------------------------------------------
private:

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TTitleExec(Core& core);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:

public:
    // Member access
    QString name() const override;
    QStringList members() const override;

    // Run
    void perform() override;
    void stop() override;


//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
};

#endif // T_TITLEEXEC_H
