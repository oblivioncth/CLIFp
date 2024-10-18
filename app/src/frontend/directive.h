#ifndef DIRECTIVE_H
#define DIRECTIVE_H

// Qt Includes
#include <QString>

// Qx Includes
#include <qx/core/qx-error.h>

//-Non-blocking Directives-----------------------------------------------------------------
struct DMessage
{
    QString text;
    bool selectable = false;
};

struct DError
{
    QString source;
    Qx::Error error;
};

struct DClipboardUpdate
{
    QString text;
};

using AsyncDirective = std::variant<
    DMessage,
    DError,
    DClipboardUpdate
>;

template<typename T>
concept AsyncDirectiveT = requires(AsyncDirective ad, T t) { ad = t; };

//-Blocking Directives---------------------------------------------------------------------
struct DBlockingMessage
{
    QString text;
    bool selectable = false;
};

struct DBlockingError
{
    enum class Choice {Ok, Yes, No};
    Q_DECLARE_FLAGS(Choices, Choice);

    QString source;
    Qx::Error error;
    Choices choices = Choice::Ok;
    Choice defaultChoice = Choice::No;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(DBlockingError::Choices);

struct DSaveFilename
{
    QString caption;
    QString dir;
    QString filter;
    QString* selectedFilter = nullptr;
};

struct DExistingDir
{
    QString caption;
    QString dir;
    // TODO: Make sure to use QFileDialog::ShowDirsOnly on receiving end
};

struct DItemSelection
{
    QString caption;
    QString label;
    QStringList items;
};

struct DYesOrNo
{
    QString question;
};

using SyncDirective = std::variant<
    DBlockingMessage,
    DBlockingError,
    DSaveFilename,
    DExistingDir,
    DItemSelection,
    DYesOrNo
>;

template<typename T>
concept SyncDirectiveT = requires(SyncDirective sd, T t) { sd = t; };

//-Any---------------------------------------------------------------------
template<typename T>
concept DirectiveT = AsyncDirectiveT<T> || SyncDirectiveT<T>;

//-Metatype Declarations-----------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(AsyncDirective);
Q_DECLARE_METATYPE(SyncDirective);

#endif // DIRECTIVE_H
