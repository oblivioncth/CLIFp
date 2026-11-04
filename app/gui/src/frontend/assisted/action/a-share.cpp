// Qx Includes
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "frontend/assisted/action/action.h"
#include "frontend/assisted/dialog/titleentrydialog.h"

using namespace Qt::StringLiterals;

//===============================================================================================================
// AShare
//===============================================================================================================

/* TODO: Alter CShare so that it can genreate a link and register CLIFp as the share handler in one go. This requires
 * having the registration either do nothing if already registered, or having another command that will do it "if not already"
 * so that the normal one can still warn or whatever it does right now if its already registered.
 */

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
            command << u"share"_s;

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
