// Unit Include
#include "console.h"

// Qt Includes
#include <QGuiApplication>
#include <QFileInfo>
#include <QDir>

// Qx Includes
#include <qx/core/qx-iostream.h>
#include <qx/core/qx-string.h>

// Magic enum
#include "magic_enum.hpp"

#define ENUM_NAME(eenum) QString(magic_enum::enum_name(eenum).data())

/* NOTE: Unlike the GUI frontend, this one blocks fully when the user is prompted for input because
 * the standard cin read methods block and of course don't spin the event loop internally like
 * QMessageBox, QFileDialog, etc. do. Technically, this isn't a problem because whenever the driver
 * thread prompts for input it also blocks, but if that ever changes then console input will have
 * to be handled asynchronously using a technique like this: https://github.com/juangburgos/QConsoleListener
 */

//===============================================================================================================
// FrontendConsole
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------------
//Public:
FrontendConsole::FrontendConsole(QGuiApplication* app) :
    FrontendFramework(app)
{
    // We don't make windows in this frontend, but just to be safe
    app->setQuitOnLastWindowClosed(false);
}

//-Class Functions--------------------------------------------------------------------------------------------------------
//Private:

//-Instance Functions------------------------------------------------------------------------------------------------------
//Private:
void FrontendConsole::handleDirective(const DMessage& d)
{
    /* TODO: Look into escape codes (console support per platform varies)
     * to replicate bold/italic/underlined.
     *
     * Also, might want to replace the html tags with some kind of more
     * complex message object or just some other way to better communicate
     * the message in a frontend agnostic matter, so we could even have something
     * like <u>Underlined Text</u> for GUI and:
     *
     * Underlined Text
     * ---------------
     *
     * for console.
     */

    // Replace possible HTML tags/entities with something more sensible for console
    QString txt = Qx::String::mapArg(d.text, {
        {u"<u>"_s, u""_s},
        {u"</u>"_s, u""_s},
        {u"<b>"_s, u"*"_s},
        {u"</b>"_s, u"*"_s},
        {u"<i>"_s, u"`"_s},
        {u"</i>"_s, u"`"_s},
        {u"<br>"_s, u"\n"_s},
        {u"&lt;"_s, u"<"_s},
        {u"&gt;"_s, u">"_s},
        {u"&nbsp;"_s, u" "_s},
    });
    txt.prepend(HEADING_MESSAGE + u" "_s);
    print(txt);
}

void FrontendConsole::handleDirective(const DError& d) { print(d.error, Stream::Error);}

void FrontendConsole::handleDirective(const DProcedureStart& d)
{
    print(d.label);
    mProgressPrinter.start(d.label);
}

void FrontendConsole::handleDirective(const DProcedureStop& d)
{
    Q_UNUSED(d);
    mProgressPrinter.finish();
}

void FrontendConsole::handleDirective(const DProcedureProgress& d) { mProgressPrinter.setValue(d.current); }
void FrontendConsole::handleDirective(const DProcedureScale& d) { mProgressPrinter.setMaximum(d.max); }
void FrontendConsole::handleDirective(const DStatusUpdate& d) { print(d.heading + u"] "_s + d.message);}

// Sync directive handlers
void FrontendConsole::handleDirective(const DBlockingMessage& d)
{
    print(HEADING_MESSAGE + u" "_s + d.text);
}

// Request directive handlers
void FrontendConsole::handleDirective(const DBlockingError& d, DBlockingError::Choice* response)
{
    Q_ASSERT(d.choices != DBlockingError::Choice::NoChoice);
    bool validDef = d.choices.testFlag(d.defaultChoice);

    // Print Error
    print(d.error, Stream::Error);
    print();

    // Print Prompt
    print(validDef ? INSTR_CHOICE_SEL : INSTR_CHOICE_SEL_NO_DEF);

    // Print Choices
    QList<DBlockingError::Choice> choices;
    for(auto c : magic_enum::enum_values<DBlockingError::Choice>())
    {
        if(!d.choices.testFlag(c))
            continue;

        choices.append(c);
        QString cStr = TEMPL_OPTION.arg(choices.count()).arg(ENUM_NAME(c));
        if(validDef && c == d.defaultChoice)
            cStr += '*';
        print(cStr);
    }

    // Get selection
    auto sel = prompt<int>([c = choices.count()](int i){ return i > 0 && i <= c; });
    *response = sel ? choices[*sel - 1] : d.defaultChoice;
}

void FrontendConsole::handleDirective(const DSaveFilename& d, QString* response)
{
    // Print Prompt
    QString cap = d.caption + (!d.extFilterDesc.isEmpty() ? u" ("_s +  d.extFilterDesc + u")"_s : QString());
    QString inst = INSTR_FILE_ENTER.arg(Qx::String::join(d.extFilter, u" "_s, u"*."_s));
    print(cap);
    print(inst);

    // Get selection
    auto path = prompt<QString>([ef = d.extFilter](const QString& i){
        QFileInfo fi(i);
        return ef.contains(fi.suffix());
    });
    *response = path ? *path : QString();
}

void FrontendConsole::handleDirective(const DExistingDir& d, QString* response)
{
    // Print Prompt
    print(d.caption);
    print(INSTR_DIR_ENTER_EXIST);

    // Get selection
    auto path = prompt<QString>([](const QString& i){
        QDir dir(i);
        return dir.exists();
    });
    *response = path ? *path: QString();
}

void FrontendConsole::handleDirective(const DItemSelection& d, QString* response)
{
    Q_ASSERT(!d.items.isEmpty());

    // Print Prompt
    print(d.caption);
    print(d.label);
    print(INSTR_ITEM_SEL);

    // Print Choices
    for(auto i = 0; const QString& itm : d.items)
        print(TEMPL_OPTION.arg(++i).arg(itm));

    // Get selection
    auto sel = prompt<int>([c = d.items.count()](int i){ return i > 0 && i <= c; });
    *response = sel ? d.items[*sel - 1] : d.items.first();
}

void FrontendConsole::handleDirective(const DYesOrNo& d, bool* response)
{
    // Print Prompt
    print(d.question);
    print(INSTR_YES_OR_NO);

    // Get selection
    static QSet<QString> yes{u"yes"_s, u"y"_s};
    static QSet<QString> no{u"no"_s, u"n"_s};
    auto answer = prompt<QString>([](const QString& i){
        QString lc = i.toLower();
        return yes.contains(lc) || no.contains(lc);
    });
    *response = answer && yes.contains(answer->toLower());
}

template<typename P>
    requires Qx::defines_left_shift_for<QTextStream, P>
void FrontendConsole::print(const P& p, Stream s)
{
    auto& cs = s == Stream::Out ? Qx::cout : Qx::cerr;

    mProgressPrinter.checkAndResetEndlCtrl();
    cs << p << Qt::endl;
}

template<Input::Type R, typename V>
    requires Input::Validator<R, V>
std::optional<R> FrontendConsole::prompt(V v)
{
    forever
    {
        QString str = Qx::cin.readLine();
        if(str.isEmpty())
            return std::nullopt;

        R convIn;
        if(Input::to(str, convIn) && v(convIn))
            return convIn;

        print(INSTR_TRY_AGAIN);
    }
}
