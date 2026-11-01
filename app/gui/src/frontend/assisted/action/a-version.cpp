// Project Includes
#include "frontend/assisted/action/action.h"

using namespace Qt::StringLiterals;

//===============================================================================================================
// AVersion
//===============================================================================================================

class AVersion : public Action
{
//-Instance Functions------------------------------------------------------------------------------------------------
public:
    DAssistedMode::Action perform() override { return DAssistedMode::SpecialAction::Version; }
};
REGISTER_ACTION(AVersion, u"View Version Info"_s, 102)
