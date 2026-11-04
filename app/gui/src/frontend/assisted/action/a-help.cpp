// Project Includes
#include "frontend/assisted/action/action.h"

//===============================================================================================================
// AHelp
//===============================================================================================================

class AHelp : public Action
{
//-Instance Functions------------------------------------------------------------------------------------------------
public:
    DAssistedMode::Action perform() override { return DAssistedMode::SpecialAction::Help; }
};
REGISTER_ACTION(AHelp, u"View Help"_s, 101)
