// Qx Includes
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "frontend/assisted/action/action.h"
#include "frontend/assisted/dialog/titleentrydialog.h"

using namespace Qt::StringLiterals;

/* TODO: If this feautre is every seriously expanded on, we should probably make a "title-action" intermediary
 * similar to title-command, that this, AShortcut, and anything else relavent can inherit from to reduce
 * redundancy
 */

//===============================================================================================================
// APlay
//===============================================================================================================

class APlay : public Action
{
//-Instance Functions------------------------------------------------------------------------------------------------
public:
    DAssistedMode::Action perform() override
    {
        QStringList command; // Empty means no action

        std::optional<TitleEntryDialog::Entry> sel = TitleEntryDialog::getTitle(nullptr, u"Title to Play"_s);
        if(sel)
        {
            command << u"play"_s;

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
REGISTER_ACTION(APlay, u"Play a Game/Animation"_s, 1)
