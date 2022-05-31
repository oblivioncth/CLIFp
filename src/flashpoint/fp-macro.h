#ifndef FLASHPOINT_MACRO_H
#define FLASHPOINT_MACRO_H

#include <QHash>

namespace Fp
{

class MacroResolver
{
//-Inner Classes----------------------------------------------------------------------------------------------------
public:
    class Key
    {
        friend class Install;
    private:
        Key() {};
        Key(const Key&) = default;
    };

//-Class Variables-----------------------------------------------------------------------------------------------
private:
    static inline const QString FP_PATH = "<fpPath>";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QHash<QString, QString> mMacroMap; // Will make sense if more macros are added

//-Constructor-------------------------------------------------------------------------------------------------
public:
    MacroResolver(QString installPath, const Key&); // Will need to be improved if many more macros are added

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString resolve(QString macroStr) const;
};

}

#endif // FLASHPOINT_MACRO_H
