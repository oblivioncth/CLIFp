// Unit Include
#include "input.h"

namespace
{

template<typename MF, typename T>
bool numericConversion(const QString& s, MF&& mf, T& out)
{
    bool ok;
    if constexpr(std::integral<T>)
        out = std::invoke(std::forward<MF>(mf), s, &ok, 10);
    else
        out = std::invoke(std::forward<MF>(mf), s, &ok);
    return ok;
}

}

namespace Input
{

//-Functions------------------------------------------------------------------------------------------------------
bool to(const QString& in, QString& out) { out = in; return true; }
bool to(const QString& in, int& out) { return numericConversion(in, &QString::toInt, out); }
bool to(const QString& in, double& out) { return numericConversion(in, &QString::toDouble, out); }

}
