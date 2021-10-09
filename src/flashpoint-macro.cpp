#include "flashpoint-macro.h"

namespace FP
{

//===============================================================================================================
// MACRO_RESOLVER
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
MacroResolver::MacroResolver(QString installPath, const Key&)
{
    mMacroMap[FP_PATH] = installPath;
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QString MacroResolver::resolve(QString macroStr) const
{
    QHash<QString, QString>::const_iterator i;
    for(i = mMacroMap.constBegin(); i != mMacroMap.constEnd(); i++)
        macroStr.replace(i.key(), i.value());

    return macroStr;
}

}
