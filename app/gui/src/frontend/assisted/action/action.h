#ifndef ACTION_H
#define ACTION_H

// Standard Library Includes
#include <algorithm>
#include <memory>

// Qt Includes
#include <QString>
#include <QList>

// Project Includes
#include "kernel/directive.h"

class Action
{
public:
    virtual ~Action() = default;
    virtual DAssistedMode::Action perform() = 0;
};

class ActionRegistry
{
//-Class Types------------------------------------------------------------------------------------------------
public:
    using Factory = std::unique_ptr<Action> (*)();

    struct Entry
    {
        QString text;
        int order;
        Factory factory;

        bool operator<(const Entry& other) const
        {
            return order != other.order ? order < other.order : text < other.text;
        }
    };

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QList<Entry> mEntries;
    bool mSorted;

//-Constructor----------------------------------------------------------------------------------------------------------
private:
    ActionRegistry() = default;

//-Class Functions------------------------------------------------------------------------------------------------
public:
    // Function-local static: avoid SIOF
    static ActionRegistry& instance()
    {
        static ActionRegistry registry;
        return registry;
    }

//-Instance Functions------------------------------------------------------------------------------------------------
public:
    // Returns true as an arbitrary value for the registration variable created when this is used
    bool add(const QString& text, int order, Factory factory)
    {
        mSorted = false;
#if defined(__cpp_aggregate_paren_init) && __cpp_aggregate_paren_init >= 201907L
        mEntries.emplaceBack(text, order, factory);
#else
        // Older Clang doesn't support P0960R3 which is triggered via emplaceBack()
        mEntries.append(Entry{text, order, factory});
#endif
        return true;
    }

    // Static init order across TUs is unspecified, so sort for a stable menu.
    const QList<Entry>& entries()
    {
        if(!mSorted)
        {
            std::sort(mEntries.begin(), mEntries.end());
            mSorted = true;
        }

        return mEntries;
    }
};

#define REGISTER_ACTION(Class, actionText, actionOrder) \
    inline const bool g_registered_##Class = ActionRegistry::instance().add( \
                                      actionText, actionOrder, []() -> std::unique_ptr<Action> { return std::make_unique<Class>(); });


#endif // ACTION_H
