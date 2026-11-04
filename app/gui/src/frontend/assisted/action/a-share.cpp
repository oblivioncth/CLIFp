// Qx Includes
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "frontend/assisted/action/action.h"
#include "frontend/assisted/dialog/titleentrydialog.h"

using namespace Qt::StringLiterals;

//===============================================================================================================
// AShare
//===============================================================================================================

class AShare : public Action
{
//-Instance Functions------------------------------------------------------------------------------------------------
public:
    DAssistedMode::Action perform() override
    {
        QStringList command; // Empty means no action

        std::optional<TitleEntryDialog::Entry> sel = TitleEntryDialog::getTitle(nullptr, u"Title to Share"_s);
        if(sel)
        {
            command << u"share"_s << u"-C"_s; // Ensure share link handling is setup

            std::visit(qxFuncAggregate{
                [&command](TitleEntryDialog::IdEntry ide) { command << u"-i"_s << ide.id; },
                [&command](TitleEntryDialog::TitleEntry te){
                    command << u"-t"_s << te.title;
                    if(!te.subtitle.isEmpty())
                        command << u"-s"_s << te.subtitle;
                },
            }, sel.value());
        }

        return command;
    }
};
REGISTER_ACTION(AShare, u"Create a Share Link"_s, 3)
