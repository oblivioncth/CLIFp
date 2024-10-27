// Unit Include
#include "errorstatus.h"

//===============================================================================================================
// ErrorStatus
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
ErrorStatus::ErrorStatus() :
    Qx::SetOnce<Qx::Error, bool (*)(const Qx::Error&, const Qx::Error&)>(Qx::Error(), &compare)
{}

//-Class Functions-------------------------------------------------------------
//Private:
bool ErrorStatus::compare(const Qx::Error& a, const Qx::Error& b)
{
    return a.value() == b.value();
}

//-Instance Functions-------------------------------------------------------------
//Public:
ErrorStatus& ErrorStatus::operator=(const Qx::Error& value)
{
    Qx::SetOnce<Qx::Error, bool (*)(const Qx::Error&, const Qx::Error&)>::operator=(value);
    return *this;
}
