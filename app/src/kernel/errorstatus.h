#ifndef ERRORSTATUS_H
#define ERRORSTATUS_H

// Qx Includes
#include <qx/core/qx-setonce.h>
#include <qx/core/qx-error.h>
//static inline const auto compare = [](const Qx::Error& a, const Qx::Error& b){ return a == b; };

class ErrorStatus : public Qx::SetOnce<Qx::Error, bool (*)(const Qx::Error&, const Qx::Error&)>
{
//-Constructor--------------------------------------------------------------------
public:
    ErrorStatus();

//-Class Functions----------------------------------------------------------------
private:
    static bool compare(const Qx::Error& a, const Qx::Error& b);

//-Instance Functions-------------------------------------------------------------
public:
    ErrorStatus& operator=(const Qx::Error& value);
};

#endif // ERRORSTATUS_H
