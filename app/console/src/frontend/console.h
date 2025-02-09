#ifndef CONSOLE_H
#define CONSOLE_H

// Qx Includes
#include <qx/utility/qx-concepts.h>

// Project Includes
#include "frontend/framework.h"
#include "frontend/progressprinter.h"
#include "input.h"

class FrontendConsole final : public FrontendFramework
{
//-Class Enums--------------------------------------------------------------------------------------------------------
private:
    enum class Stream { Out, Error };

//-Class Variables--------------------------------------------------------------------------------------------------------
private:
    // Heading
    static inline const QString HEADING_MESSAGE = u"Notice)"_s;

    // Instructions
    static inline const QString INSTR_TRY_AGAIN = u"Invalid response, try again..."_s;
    static inline const QString INSTR_CHOICE_SEL = u"Select Action (or ENTER for *):"_s;
    static inline const QString INSTR_CHOICE_SEL_NO_DEF = u"Select Action:"_s;
    static inline const QString INSTR_FILE_ENTER = u"File Path (or ENTER to cancel):"_s;
    static inline const QString INSTR_DIR_ENTER_EXIST = u"Existing Directory (or ENTER to cancel):"_s;
    static inline const QString INSTR_YES_OR_NO = u"Yes or No/ENTER:"_s;
    static inline const QString INSTR_ITEM_SEL = u"Select Item (or ENTER for 1):"_s;

    // Templates
    static inline const QString TEMPL_OPTION = u"%1) %2"_s;

//-Instance Variables----------------------------------------------------------------------------------------------
private:
    ProgressPrinter mProgressPrinter;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    explicit FrontendConsole(QGuiApplication* app);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Async directive handlers
    void handleDirective(const DMessage& d) override;
    void handleDirective(const DError& d) override;
    void handleDirective(const DProcedureStart& d) override;
    void handleDirective(const DProcedureStop& d) override;
    void handleDirective(const DProcedureProgress& d) override;
    void handleDirective(const DProcedureScale& d) override;
    void handleDirective(const DStatusUpdate& d) override;

    // Sync directive handlers
    void handleDirective(const DBlockingMessage& d) override;

    // Request directive handlers
    void handleDirective(const DBlockingError& d, DBlockingError::Choice* response) override;
    void handleDirective(const DSaveFilename& d, QString* response) override;
    void handleDirective(const DExistingDir& d, QString* response) override;
    void handleDirective(const DItemSelection& d, QString* response) override;
    void handleDirective(const DYesOrNo& d, bool* response) override;

    // Derived
    template<typename P = QString>
        requires Qx::defines_left_shift_for<QTextStream, P>
    void print(const P& p = QString(), Stream s = Stream::Out);

    template<Input::Type R, typename V>
        requires Input::Validator<R, V>
    std::optional<R> prompt(V v);
};

#endif // CONSOLE_H
