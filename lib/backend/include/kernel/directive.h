#ifndef DIRECTIVE_H
#define DIRECTIVE_H

// Shared Library Support
#include "clifp_backend_export.h"

// Qt Includes
#include <QString>

// Qx Includes
#include <qx/core/qx-error.h>

// Shared-lib

/* TODO:
 *
 * In this file there are some structs with redundant members where one could easily conceive
 * that they should instead be part of an inheritance chain (i.e. DBlockingError inheriting
 * from DError); however, C++ currently does not allow using designated-initializers for
 * base classes, so even though with this inheritance chain:
 *
 * struct A { int one; }
 * struct B { int two; }
 *
 * You can do:
 *
 * B b; b.one = 1; b.two = 2;
 *
 * You cannot do:
 * B b{ .one =1, b.two = 2; }
 *
 * We very much would like to support the latter syntax for all directive creation, so that means
 * we cannot use base classes for them. P2287 has be proposed to allow this:
 * https://github.com/cplusplus/papers/issues/978
 *
 * If it's accepted, once it's standardized we can switch to using it, but that won't be for some time,
 * so in the meanwhile we keep the redundant members and use template functions where a function
 * could have instead taken a pointer to base.
 */

//-Non-blocking Directives-----------------------------------------------------------------
struct DMessage
{
    QString text;
    bool selectable = false;
};

struct DError
{
    Qx::Error error;
};

struct DProcedureStart
{
    QString label;
};

struct DProcedureStop {};

struct DProcedureProgress
{
    quint64 current;
};

struct DProcedureScale
{
    quint64 max;
};

struct DClipboardUpdate
{
    QString text;
};

struct DStatusUpdate
{
    QString heading;
    QString message;
};

using AsyncDirective = std::variant<
    DMessage,
    DError,
    DProcedureStart,
    DProcedureStop,
    DProcedureProgress,
    DProcedureScale,
    DClipboardUpdate,
    DStatusUpdate
>;

template<typename T>
concept AsyncDirectiveT = requires(AsyncDirective ad, T t) { ad = t; };

//-Blocking Directives---------------------------------------------------------------------
struct DBlockingMessage
{
    QString text;
    bool selectable = false;
};

using SyncDirective = std::variant<DBlockingMessage>;

template<typename T>
concept SyncDirectiveT = requires(SyncDirective sd, T t) { sd = t; };

//-Blocking Directives With Response-------------------------------------------------------
struct DBlockingError
{
    enum class Choice
    {
        NoChoice = 0x0,
        Ok = 0x1,
        Yes = 0x2,
        No = 0x4
    };
    Q_DECLARE_FLAGS(Choices, Choice);

    Qx::Error error;
    Choices choices = Choice::Ok;
    Choice defaultChoice = Choice::No;

    using response_type = Choice;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(DBlockingError::Choices);

struct DSaveFilename
{
    QString caption;
    QString dir;
    QString filter;
    QString* selectedFilter = nullptr;

    using response_type = QString;
};

struct DExistingDir
{
    QString caption;
    QString startingDir;

    using response_type = QString;
};

struct DItemSelection
{
    QString caption;
    QString label;
    QStringList items;

    using response_type = QString;
};

struct DYesOrNo
{
    QString question;

    using response_type = bool;
};

using RequestDirective = std::variant<
    DBlockingError,
    DSaveFilename,
    DExistingDir,
    DItemSelection,
    DYesOrNo
>;

template<typename T>
concept RequestDirectiveT = requires(RequestDirective rd, T t) { rd = t; typename T::response_type; } &&
                            !std::same_as<typename T::response_type, void>;

//-Any---------------------------------------------------------------------
template<typename T>
concept DirectiveT = AsyncDirectiveT<T> || SyncDirectiveT<T> || RequestDirectiveT<T>;

//-Metatype Declarations-----------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(AsyncDirective);
Q_DECLARE_METATYPE(SyncDirective);
Q_DECLARE_METATYPE(RequestDirective);

#endif // DIRECTIVE_H
