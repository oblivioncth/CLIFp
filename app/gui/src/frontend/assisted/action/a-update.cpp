// Project Includes
#include "frontend/assisted/action/action.h"

using namespace Qt::StringLiterals;

//===============================================================================================================
// AUpdate
//===============================================================================================================

class AUpdate : public Action
{
//-Instance Functions------------------------------------------------------------------------------------------------
public:
    DAssistedMode::Action perform() override { return QStringList({u"update"_s}); }
};
REGISTER_ACTION(AUpdate, u"Update CLIFp"_s, 100)
